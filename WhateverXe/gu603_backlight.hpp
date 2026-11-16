#ifndef GU603_BACKLIGHT_HPP
#define GU603_BACKLIGHT_HPP

#include <stdint.h>

#ifdef KERNEL
#include <IOKit/IORegistryEntry.h>
#include <os/log.h>
#endif

namespace GU603Backlight {

inline bool eligible(bool requested, uint32_t identity, uint32_t revision) {
    return requested && identity == 0xA7A88086U && revision == 4;
}

// Apple's AppleIntelFramebuffer::{get,set}AttributeForConnection answer the 'bklt'
// attribute with kIOReturnUnsupported (0xE00002C7) unless fb index == 0 and the
// controller's "has built-in port" byte (controller+0xE55, set once in
// Controller::start from AppleIntelPort::getBuiltInPort()!=NULL) is non-zero.
// It is zero on this machine. Consequence (AppleBacklight AppleIntelPanelA::setDisplay):
// at display attach it asks getAttributeForConnection(0,'bklt'); "unsupported" makes
// it fall back to its AGDC route (+0x120, which ends at NVIDIA: "no internal display
// answered the brightness query") and the slider never reaches the Intel framebuffer
// (Trial12 saw no 'bklt' set at all). Trial13 therefore (1) answers the 'bklt' getter
// itself when native refuses, so AppleIntelPanelA selects the framebuffer, and
// (2) handles the 'bklt' setter (PWM path below).
static constexpr uint32_t kAttributeBKLT = 0x626B6C74U; // 'bklt'
static constexpr uint32_t kFramebufferPowerOn = 2;      // AppleIntelFramebuffer+0x4284

enum class AttributeAction { Native, Defer, Write };

// Native: let Apple's code run unchanged. Defer: panel not powered, remember the
// level for Apple's wake restore but touch no register. Write: program the PWM duty.
inline AttributeAction attributeAction(bool eligibleNow, uint32_t attribute, bool haveController,
                                       bool haveAccess, uint32_t framebufferIndex, uint32_t powerState) {
    if (!eligibleNow || attribute != kAttributeBKLT || !haveController || !haveAccess ||
        framebufferIndex != 0) return AttributeAction::Native;
    return powerState == kFramebufferPowerOn ? AttributeAction::Write : AttributeAction::Defer;
}

// Getter: override only when native refused ('bklt', fb index 0, controller and
// register access present). Anything else keeps Apple's answer untouched.
inline bool getterNeedsOverride(bool eligibleNow, uint32_t attribute, bool haveController,
                                bool haveAccess, uint32_t framebufferIndex, uint32_t nativeResult) {
    return eligibleNow && attribute == kAttributeBKLT && haveController && haveAccess &&
           framebufferIndex == 0 && nativeResult != 0;
}

// Current level (0..65535) as seen in the PWM registers, or `fallback` when the
// registers are unreadable/unprogrammed.
inline uint32_t levelFromRegisters(uint32_t period, uint32_t duty, uint32_t fallback) {
    if (!period || period == ~0U || duty == ~0U || duty > period) return fallback;
    return uint32_t((uint64_t(duty) * 65535U + period / 2) / period);
}

// Native getter layout: {value, min, max} as three uintptr_t.
inline void fillAttribute(uintptr_t *out, uint32_t level) {
    out[0] = level;
    out[1] = 0;
    out[2] = 0xFFFF;
}

inline uint32_t calculateDuty(uint32_t period, uint32_t brightness) {
    if (!period || period == ~0U) return 0;
    if (brightness > 65535) brightness = 65535;
    return uint32_t((uint64_t(period) * brightness) / 65535U);
}

inline uint32_t fallbackPeriod(uint32_t strap, uint32_t vbtHz) {
    const uint32_t clock = (strap & 256) ? 24000000U : 19200000U;
    const uint32_t hz = vbtHz ? vbtHz : 1000U;
    return (clock + hz / 2) / hz;
}

template <class ReadFn, class WriteFn>
inline uint32_t setBacklight(void *controller, void *access, uint32_t brightness,
                             uint32_t vbtHz, ReadFn readReg, WriteFn writeReg) {
    if (!controller || !access) return 0xE00002EBU; // kIOReturnNotReady

    if (brightness > 65535) brightness = 65535;
    auto c = static_cast<uint8_t *>(controller);

    // Never let a stray level 0 (e.g. an unset "restore" value during attach) black
    // the panel before any real level has been applied. Later zeros (display sleep)
    // are honoured. Remember the request so Apple's wake restore still sees it.
    static bool litOnce = false;
    if (brightness == 0 && !litOnce) {
        *reinterpret_cast<uint32_t *>(c + 0xE58) = 0;
        return 0;
    }

    // Follow whatever period the hardware is actually running (firmware's or Apple's
    // LightUpEDP rewrite); re-read on every write so a later period change cannot
    // leave a stale scale. Program the fallback only when the register is empty.
    static uint32_t cachedPeriod = 0;
    const uint32_t hwPeriod = readReg(0xC8254);
    if (hwPeriod && hwPeriod != ~0U) {
        cachedPeriod = hwPeriod;
    } else {
        if (!cachedPeriod || cachedPeriod == ~0U) cachedPeriod = fallbackPeriod(readReg(0xC2014), vbtHz);
        if (hwPeriod == 0) writeReg(0xC8254, cachedPeriod);
    }
    const uint32_t duty = calculateDuty(cachedPeriod, brightness);
    if (brightness != 0) litOnce = true;

    writeReg(0xC8258, duty);

    const uint32_t ctl = readReg(0xC8250);
    if (ctl != ~0U && !(ctl & 0x80000000U)) {
        writeReg(0xC8250, ctl | 0x80000000U);
    }

    *reinterpret_cast<uint32_t *>(c + 0xE58) = brightness;
    *reinterpret_cast<uint32_t *>(c + 0xE60) = cachedPeriod;

#ifdef KERNEL
    static uint32_t callCount = 0;
    const uint32_t call = __sync_add_and_fetch(&callCount, 1);
    if (call <= 16 || (call % 64) == 0) {
        os_log(OS_LOG_DEFAULT, "GU603 trial13 blfix call=%u brightness=%u period=%u duty=%u ctl=0x%x\n",
               call, brightness, cachedPeriod, duty, ctl);
    }
    uint32_t receipt[] = {1, call, brightness, cachedPeriod, duty, ctl};
    IORegistryEntry::getRegistryRoot()->setProperty("GU603BacklightFix", receipt, sizeof(receipt));
#endif

    return 0; // kIOReturnSuccess
}

template <class WriteFn>
inline void enablePWM(void *access, WriteFn writeReg) {
    if (access) {
        writeReg(0xC8250, 0x80000000U);
    }
}

template <class ReadFn>
inline void lightUp(void *controller, void *access, uint32_t vbtHz, ReadFn readReg) {
    if (!controller || !access) return;
    auto c = static_cast<uint8_t *>(controller);
    uint32_t p = readReg(0xC8254);
    if (!p || p == ~0U) {
        p = fallbackPeriod(readReg(0xC2014), vbtHz);
    }
    *reinterpret_cast<uint32_t *>(c + 0xE60) = p;
}

} // namespace GU603Backlight

#endif // GU603_BACKLIGHT_HPP
