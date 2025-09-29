#ifndef RTC_H
#define RTC_H

#include <stdint.h>

#define RTC_SECONDS 0x00
#define RTC_MINUTES 0x02
#define RTC_HOURS 0x04
#define RTC_DAY 0x07
#define RTC_MONTH 0x08
#define RTC_YEAR 0x09

extern uint8_t _readTime(uint8_t reg);
uint8_t getTime(uint8_t reg);

#endif 