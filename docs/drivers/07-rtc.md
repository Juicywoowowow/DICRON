Real-Time Clock (RTC)
=====================

1. Overview
-----------

The Real-Time Clock (RTC) provides time-of-day information and
maintains the system's understanding of current time even when
the system is powered off.

2. I/O Ports
------------

The RTC uses CMOS RAM and is accessed via:
  0x70 - Index Register (write)
  0x71 - Data Register (read/write)

3. RTC Registers
----------------

  0x00 - Seconds (0-59)
  0x01 - Seconds Alarm
  0x02 - Minutes (0-59)
  0x03 - Minutes Alarm
  0x04 - Hours (0-23)
  0x05 - Hours Alarm
  0x06 - Day of Week (1-7)
  0x07 - Date of Month (1-31)
  0x08 - Month (1-12)
  0x09 - Year (0-99)
  0x0A - Register A (status)
  0x0B - Register B (control)
  0x0C - Register C (status)
  0x0D - Register D (status)

4. Register A
-------------

  Bit 7: Update In Progress (UIP)
  Bits 6-4: Divider (010 = 32.768 kHz)
  Bits 3-0: Rate (0110 = 122 ms, default for periodic)

5. Register B
-------------

  Bit 7: Set (1 = disable updates)
  Bit 6: PIE (Periodic Interrupt Enable)
  Bit 5: AIE (Alarm Interrupt Enable)
  Bit 4: UIE (Update End Interrupt Enable)
  Bit 3: Square Wave Enable
  Bit 2: Binary Mode (0 = BCD, 1 = Binary)
  Bit 1: 24/12 Hour (0 = 12 hour, 1 = 24 hour)
  Bit 0: Daylight Savings

6. API
-----

  void rtc_init(void)
      - Initialize RTC driver

  void rtc_read(struct rtc_time *t)
      - Read current time into rtc_time structure

  uint64_t rtc_unix_time(void)
      - Returns seconds since Unix epoch (1970-01-01)

7. Time Structure
-----------------

  struct rtc_time {
      uint8_t second;   - Seconds (0-59)
      uint8_t minute;   - Minutes (0-59)
      uint8_t hour;     - Hours (0-23)
      uint8_t day;      - Day of month (1-31)
      uint8_t month;    - Month (1-12)
      uint16_t year;    - Year (full year)
  };

8. BCD Conversion
-----------------

The RTC stores time in BCD (Binary Coded Decimal). Values must be
converted to binary:
  binary = (bcd & 0x0F) + ((bcd >> 4) * 10)

9. Limitations
--------------

  - Year only stores two digits (assumes 2000-2099)
  - No leap second handling
  - CMOS battery required for persistent time
