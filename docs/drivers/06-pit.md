Programmable Interval Timer (PIT)
================================

1. Overview
-----------

The PIT (Programmable Interval Timer) provides timer interrupts for the
scheduler. It is typically used to generate interrupts at a regular
interval (1000 Hz = 1 ms per tick).

2. I/O Ports
------------

  0x40 - Channel 0 Data Port
  0x41 - Channel 1 Data Port
  0x42 - Channel 2 Data Port
  0x43 - Mode/Command Register

3. Mode/Command Register
-------------------------

  Bits 7-6: Select channel (00=ch0, 01=ch1, 10=ch2, 11=read-back)
  Bits 5-4: Read/Write select (00=latch count, 01=lsb only, 10=msb only, 11=lsb then msb)
  Bits 3-1: Mode (0=interrupt on terminal count, 2=rate generator, 3=square wave)
  Bit 0: BCD/binary (0=16-bit binary, 1=BCD)

4. Configuration
----------------

For 1000 Hz (1 ms tick):
  - Divisor = 1193182 / 1000 = 1193
  - Command: 0x36 (channel 0, lsb/msb, rate generator, binary)
  - Write 1193 to port 0x40 (low byte first, then high byte)

5. API
-----

  void pit_init(void)
      - Initialize PIT to 1000 Hz
      - Only compiled in if CONFIG_PIT is set

6. Usage in Scheduler
----------------------

The PIT is the primary timer source for preemptive scheduling:
  - Each interrupt triggers preempt_tick()
  - Tracks remaining timeslice for current task
  - Triggers reschedule when timeslice expires

7. Interrupt Handling
-------------------

The PIT uses IRQ 0. The IDT handler for this vector:
  - Acknowledges the interrupt
  - Calls preempt_tick()
  - May trigger scheduler reschedule
