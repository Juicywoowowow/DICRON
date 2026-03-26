#ifndef _DICRON_DRIVERS_TIMER_RTC_H
#define _DICRON_DRIVERS_TIMER_RTC_H

#include <stdint.h>

struct rtc_time {
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint8_t month;
	uint16_t year;
};

void rtc_init(void);
void rtc_read(struct rtc_time *t);

/* Returns seconds since Unix epoch (1970-01-01 00:00:00 UTC) */
uint64_t rtc_unix_time(void);

#endif
