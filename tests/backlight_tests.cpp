#include <cassert>
#include <cstdio>
#include <cstdint>
#include <map>
#include "gu603_backlight.hpp"

int main() {
    // 1. Test eligibility logic
    assert(GU603Backlight::eligible(true, 0xA7A88086U, 4));
    assert(!GU603Backlight::eligible(false, 0xA7A88086U, 4));
    assert(!GU603Backlight::eligible(true, 0x46A68086U, 4));
    assert(!GU603Backlight::eligible(true, 0xA7A88086U, 0));

    // 2. Test calculateDuty
    // Period = 24000 (1000 Hz with 24 MHz clock)
    uint32_t period = 24000;
    assert(GU603Backlight::calculateDuty(period, 65535) == 24000);
    assert(GU603Backlight::calculateDuty(period, 0) == 0);
    assert(GU603Backlight::calculateDuty(period, 32767) == 11999 || GU603Backlight::calculateDuty(period, 32767) == 12000);
    assert(GU603Backlight::calculateDuty(period, 70000) == 24000); // clamped to 65535
    assert(GU603Backlight::calculateDuty(0, 32768) == 0);

    // 3. Test fallbackPeriod
    assert(GU603Backlight::fallbackPeriod(256, 1000) == 24000); // strap bit 8 set -> 24 MHz
    assert(GU603Backlight::fallbackPeriod(0, 1000) == 19200);   // strap bit 8 clear -> 19.2 MHz
    assert(GU603Backlight::fallbackPeriod(256, 0) == 24000);    // default 1000 Hz

    // 4. Test setBacklight mock sequence
    uint8_t mockController[0x2000] = {0};
    uint8_t mockAccess[0x100] = {0};
    std::map<unsigned long, uint32_t> registers;
    registers[0xC8254] = 24000; // HW reports 24000 period
    registers[0xC8250] = 0x80000000; // PWM already enabled
    registers[0xC8258] = 12000;

    auto readFn = [&](unsigned long reg) -> uint32_t {
        return registers[reg];
    };
    auto writeFn = [&](unsigned long reg, uint32_t val) {
        registers[reg] = val;
    };

    // Test 100% brightness (65535)
    uint32_t ret = GU603Backlight::setBacklight(mockController, mockAccess, 65535, 1000, readFn, writeFn);
    assert(ret == 0);
    assert(registers[0xC8258] == 24000);
    assert(*reinterpret_cast<uint32_t *>(mockController + 0xE58) == 65535);
    assert(*reinterpret_cast<uint32_t *>(mockController + 0xE60) == 24000);

    // Test 30% brightness (approx 19660 / 65535)
    ret = GU603Backlight::setBacklight(mockController, mockAccess, 19660, 1000, readFn, writeFn);
    assert(ret == 0);
    uint32_t expectedDuty = uint32_t((uint64_t(24000) * 19660) / 65535);
    assert(registers[0xC8258] == expectedDuty);
    assert(*reinterpret_cast<uint32_t *>(mockController + 0xE58) == 19660);

    // Test 0% brightness
    ret = GU603Backlight::setBacklight(mockController, mockAccess, 0, 1000, readFn, writeFn);
    assert(ret == 0);
    assert(registers[0xC8258] == 0);
    assert(*reinterpret_cast<uint32_t *>(mockController + 0xE58) == 0);

    // Test PWM enable when bit 31 was cleared
    registers[0xC8250] = 0x00000000;
    ret = GU603Backlight::setBacklight(mockController, mockAccess, 32768, 1000, readFn, writeFn);
    assert(ret == 0);
    assert((registers[0xC8250] & 0x80000000U) != 0);

    // Test enablePWM
    registers[0xC8250] = 0;
    GU603Backlight::enablePWM(mockAccess, writeFn);
    assert(registers[0xC8250] == 0x80000000U);

    // Test lightUp initializes period
    *reinterpret_cast<uint32_t *>(mockController + 0xE60) = 0;
    GU603Backlight::lightUp(mockController, mockAccess, 1000, readFn);
    assert(*reinterpret_cast<uint32_t *>(mockController + 0xE60) == 24000);

    // Null checks
    assert(GU603Backlight::setBacklight(nullptr, mockAccess, 32768, 1000, readFn, writeFn) == 0xE00002EBU);
    assert(GU603Backlight::setBacklight(mockController, nullptr, 32768, 1000, readFn, writeFn) == 0xE00002EBU);

    printf("PASS: All GU603Backlight unit tests passed!\n");
    return 0;
}
