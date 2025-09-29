#include "rtc.h"

uint8_t getTime(uint8_t reg) {
    if (reg != RTC_SECONDS && reg != RTC_MINUTES && reg != RTC_HOURS &&
        reg != RTC_DAY && reg != RTC_MONTH && reg != RTC_YEAR) {
        return 0;
    }
    uint8_t value = _readTime(reg);
    return ((value >> 4) * 10) + (value & 0x0F);
}