#include "rtc.h"
#include "arch/x86_64/io.h"

#define CMOS_ADDR 0x70
#define CMOS_DATA 0x71

#define RTC_REG_SEC    0x00
#define RTC_REG_MIN    0x02
#define RTC_REG_HOUR   0x04
#define RTC_REG_DAY    0x07
#define RTC_REG_MONTH  0x08
#define RTC_REG_YEAR   0x09
#define RTC_REG_STATUS_A 0x0A
#define RTC_REG_STATUS_B 0x0B

static uint8_t cmos_read(uint8_t reg)
{
	outb(CMOS_ADDR, reg);
	return inb(CMOS_DATA);
}

static int rtc_updating(void)
{
	return cmos_read(RTC_REG_STATUS_A) & 0x80;
}

static uint8_t bcd_to_bin(uint8_t val)
{
	return (uint8_t)((val & 0x0F) + ((val >> 4) * 10));
}

void rtc_init(void)
{
	/* Nothing to configure — CMOS RTC runs autonomously */
}

void rtc_read(struct rtc_time *t)
{
	/* Wait for any in-progress update to finish */
	while (rtc_updating())
		;

	t->second = cmos_read(RTC_REG_SEC);
	t->minute = cmos_read(RTC_REG_MIN);
	t->hour   = cmos_read(RTC_REG_HOUR);
	t->day    = cmos_read(RTC_REG_DAY);
	t->month  = cmos_read(RTC_REG_MONTH);
	t->year   = cmos_read(RTC_REG_YEAR);

	uint8_t status_b = cmos_read(RTC_REG_STATUS_B);

	/* Convert BCD to binary if not already binary mode */
	if (!(status_b & 0x04)) {
		t->second = bcd_to_bin(t->second);
		t->minute = bcd_to_bin(t->minute);
		t->hour   = bcd_to_bin((uint8_t)(t->hour & 0x7F));
		t->day    = bcd_to_bin(t->day);
		t->month  = bcd_to_bin(t->month);
		t->year   = bcd_to_bin((uint8_t)t->year);
	}

	/* Handle 12-hour mode */
	if (!(status_b & 0x02) && (t->hour & 0x80)) {
		t->hour = (uint8_t)(((t->hour & 0x7F) + 12) % 24);
	}

	/* CMOS year is 2-digit, assume 2000s */
	t->year += 2000;
}

uint64_t rtc_unix_time(void)
{
	struct rtc_time t;
	rtc_read(&t);

	/* Days per month (non-leap) */
	static const uint16_t mdays[] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};

	uint64_t y = t.year;
	uint64_t days = 0;

	/* Years since 1970 */
	for (uint64_t i = 1970; i < y; i++) {
		days += 365;
		if ((i % 4 == 0 && i % 100 != 0) || i % 400 == 0)
			days++;
	}

	/* Months this year */
	if (t.month >= 1 && t.month <= 12)
		days += mdays[t.month - 1];

	/* Leap day this year */
	if (t.month > 2 &&
	    ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
		days++;

	days += (uint64_t)t.day - 1;

	return days * 86400 + (uint64_t)t.hour * 3600 +
	       (uint64_t)t.minute * 60 + t.second;
}
