// CPU-only checks for the GU603 trial13 backlight policy (no kernel, no MMIO).
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include "gu603_backlight.hpp"
using namespace GU603Backlight;

int main(int argc, char **argv) {
    if (argc > 1 && !strcmp(argv[1], "fallback")) {
        // Fresh process: nothing cached and the period register is empty, so the
        // VBT-derived fallback (200 Hz @ 19.2 MHz) is programmed and used.
        alignas(8) static uint8_t ctl[0x1000];
        std::map<unsigned long, uint32_t> r;
        r[0xC8254] = 0; r[0xC8250] = 0x80000000U; r[0xC8258] = 0; r[0xC2014] = 0;
        auto rd2 = [&](unsigned long a) { return r.count(a) ? r[a] : 0xFFFFFFFFU; };
        auto wr2 = [&](unsigned long a, uint32_t v) { r[a] = v; };
        assert(setBacklight(ctl, ctl, 65535, 200, rd2, wr2) == 0);
        assert(r[0xC8254] == 96000 && r[0xC8258] == 96000);
        puts("gu603 backlight fallback test passed");
        return 0;
    }

    // Eligibility: only the exact A7A8 / rev 4 profile with the flag set.
    assert(eligible(true, 0xA7A88086U, 4));
    assert(!eligible(false, 0xA7A88086U, 4));
    assert(!eligible(true, 0x9A498086U, 4));
    assert(!eligible(true, 0xA7A88086U, 3));

    // Setter policy: native unless 'bklt' on fb index 0 with controller + register access.
    const uint32_t B = kAttributeBKLT, P = kFramebufferPowerOn;
    assert(B == 0x626B6C74U);
    assert(attributeAction(true, B, true, true, 0, P) == AttributeAction::Write);
    assert(attributeAction(true, B, true, true, 0, 0) == AttributeAction::Defer);   // panel off
    assert(attributeAction(true, B, true, true, 0, 1) == AttributeAction::Defer);
    assert(attributeAction(false, B, true, true, 0, P) == AttributeAction::Native); // flag off
    assert(attributeAction(true, 0x70777273U /*'pwrs'*/, true, true, 0, P) == AttributeAction::Native);
    assert(attributeAction(true, B, false, true, 0, P) == AttributeAction::Native);
    assert(attributeAction(true, B, true, false, 0, P) == AttributeAction::Native);
    assert(attributeAction(true, B, true, true, 1, P) == AttributeAction::Native);  // not fb@0

    // Getter policy: override only when native refused, for 'bklt', fb 0, flag on.
    const uint32_t kUnsupported = 0xE00002C7U;
    assert(getterNeedsOverride(true, B, true, true, 0, kUnsupported));
    assert(!getterNeedsOverride(true, B, true, true, 0, 0));                 // Apple answered
    assert(!getterNeedsOverride(false, B, true, true, 0, kUnsupported));     // flag off
    assert(!getterNeedsOverride(true, 0x626B7376U /*'bksv'*/, true, true, 0, kUnsupported));
    assert(!getterNeedsOverride(true, B, false, true, 0, kUnsupported));
    assert(!getterNeedsOverride(true, B, true, false, 0, kUnsupported));
    assert(!getterNeedsOverride(true, B, true, true, 1, kUnsupported));

    // Level from registers: proportional, rounded, clamped, fallback when unusable.
    assert(levelFromRegisters(96000, 96000, 7) == 65535);
    assert(levelFromRegisters(96000, 0, 7) == 0);
    assert(levelFromRegisters(96000, 48000, 7) == 32768);        // rounds half up
    assert(levelFromRegisters(0, 100, 7) == 7);
    assert(levelFromRegisters(~0U, 100, 7) == 7);
    assert(levelFromRegisters(96000, ~0U, 7) == 7);
    assert(levelFromRegisters(96000, 96001, 7) == 7);            // duty > period is bogus

    // Getter layout matches Apple's {value, min, max}.
    uintptr_t out[3] = {1, 2, 3};
    fillAttribute(out, 24704);
    assert(out[0] == 24704 && out[1] == 0 && out[2] == 0xFFFF);

    // Duty maths: duty = period * level / 65535, clamped, never > period.
    assert(calculateDuty(0, 1000) == 0);
    assert(calculateDuty(~0U, 1000) == 0);
    assert(calculateDuty(1000, 65535) == 1000);
    assert(calculateDuty(1000, 70000) == 1000);          // clamped
    assert(calculateDuty(96000, 0) == 0);
    assert(calculateDuty(96000, 3020) == 96000ULL * 3020 / 65535);
    assert(calculateDuty(0xFFFFFF00U, 65535) == 0xFFFFFF00U); // no 32-bit overflow

    // Fallback period: 19.2 MHz / 24 MHz strap, VBT 200 Hz (this panel's VBT value).
    assert(fallbackPeriod(0, 200) == 96000);
    assert(fallbackPeriod(256, 200) == 120000);
    assert(fallbackPeriod(0, 0) == 19200);               // VBT frequency missing -> 1 kHz

    // setBacklight against a fake register file and controller buffer.
    alignas(8) static uint8_t controller[0x1000];
    memset(controller, 0, sizeof(controller));
    std::map<unsigned long, uint32_t> regs;
    regs[0xC8254] = 96000;           // firmware period (200 Hz @ 19.2 MHz)
    regs[0xC8250] = 0x80000000U;     // firmware left PWM enabled
    regs[0xC8258] = 96000;
    auto rd = [&](unsigned long r) { return regs.count(r) ? regs[r] : 0xFFFFFFFFU; };
    auto wr = [&](unsigned long r, uint32_t v) { regs[r] = v; };
    void *access = controller;       // only needs to be non-null here
    auto E58 = [&]() { return *reinterpret_cast<uint32_t *>(controller + 0xE58); };
    auto E60 = [&]() { return *reinterpret_cast<uint32_t *>(controller + 0xE60); };

    // A stray level 0 before any real level must NOT touch the hardware.
    assert(setBacklight(controller, access, 0, 200, rd, wr) == 0);
    assert(regs[0xC8258] == 96000 && regs[0xC8254] == 96000 && regs[0xC8250] == 0x80000000U);
    assert(E58() == 0);

    assert(setBacklight(controller, access, 65403, 200, rd, wr) == 0);
    assert(regs[0xC8258] == 96000ULL * 65403 / 65535);
    assert(regs[0xC8254] == 96000);                       // period untouched
    assert((regs[0xC8250] & 0x80000000U) != 0);           // never cleared
    assert(E58() == 65403);
    assert(E60() == 96000);

    assert(setBacklight(controller, access, 3020, 200, rd, wr) == 0);
    assert(regs[0xC8258] == 96000ULL * 3020 / 65535);     // dimmer than before
    assert(regs[0xC8258] < 96000ULL * 65403 / 65535);

    // The hardware period changes underneath us (Apple's LightUpEDP rewrote it):
    // the next write must follow it, not a stale cached value.
    regs[0xC8254] = 48000;
    assert(setBacklight(controller, access, 32768, 200, rd, wr) == 0);
    assert(regs[0xC8258] == 48000ULL * 32768 / 65535);
    assert(regs[0xC8254] == 48000);                       // never rewritten when valid
    assert(E60() == 48000);

    // After a real level was applied, zero is honoured (display sleep) and PWM stays enabled.
    assert(setBacklight(controller, access, 0, 200, rd, wr) == 0);
    assert(regs[0xC8258] == 0);
    assert((regs[0xC8250] & 0x80000000U) != 0);

    regs[0xC8250] = 0;                                    // firmware left PWM off -> enable it
    assert(setBacklight(controller, access, 30000, 200, rd, wr) == 0);
    assert((regs[0xC8250] & 0x80000000U) != 0);

    // Emptied period register while a valid period was seen before: reuse the last
    // known period (and program it back), never a made-up one.
    regs[0xC8254] = 0;
    assert(setBacklight(controller, access, 65535, 200, rd, wr) == 0);
    assert(regs[0xC8254] == 48000 && regs[0xC8258] == 48000);

    // Unreadable period (all ones): do not write the period register at all.
    regs[0xC8254] = 0xFFFFFFFFU;
    assert(setBacklight(controller, access, 1000, 200, rd, wr) == 0);
    assert(regs[0xC8254] == 0xFFFFFFFFU);

    assert(setBacklight(nullptr, access, 100, 200, rd, wr) == 0xE00002EBU);
    assert(setBacklight(controller, nullptr, 100, 200, rd, wr) == 0xE00002EBU);

    puts("gu603 backlight tests passed");
    return 0;
}
