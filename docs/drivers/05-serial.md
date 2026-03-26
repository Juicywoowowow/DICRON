Serial Driver (COM Ports)
==========================

1. Overview
-----------

The serial driver provides access to COM ports (RS-232 serial). It is
used for console output when no framebuffer is available.

2. I/O Ports
------------

COM1 (default):
  0x3F8 - Data Register (R/W)
  0x3F9 - Interrupt Enable / Divisor LSB
  0x3A9 - Divisor MSB (if DLAB=1)
  0x3FA - Interrupt ID
  0x3FB - Line Control Register
  0x3FC - Modem Control Register
  0x3FD - Line Status Register
  0x3FE - Modem Status Register

Other ports (COM2-4) use sequential I/O port addresses.

3. Line Control Register (LCR)
------------------------------

  Bit 7: DLAB (Divisor Latch Access Bit)
  Bit 6: Break Control
  Bit 5: Stick Parity
  Bit 4: Even Parity
  Bit 3: Parity Enable
  Bit 2: Stop Bits (0=1, 1=2)
  Bits 1-0: Word Length (00=5, 01=6, 10=7, 11=8)

4. Line Status Register (LSR)
----------------------------

  Bit 7: Error / FIFO Empty
  Bit 6: Transmitter Empty (THRE)
  Bit 5: Transmitter Holding Empty
  Bit 4: Break Interrupt
  Bit 3: Framing Error
  Bit 2: Parity Error
  Bit 1: Overrun Error
  Bit 0: Data Ready (DR)

5. API
-----

  void serial_init(void)
      - Initialize serial port (COM1, 115200 8N1)

  void serial_putchar(char c)
      - Write a character

  void serial_write(const char *s, size_t len)
      - Write string

6. Initialization
----------------

Default configuration:
  - Port: COM1 (0x3F8)
  - Baud: 115200
  - Parity: None
  - Stop bits: 1
  - Word length: 8 bits

7. Usage
-------

The serial port is used for:
  - Early boot logging
  - Console output when framebuffer unavailable
  - Debugging

8. Header
---------

The driver header is in src/drivers/serial/com.h.
