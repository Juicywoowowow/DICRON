PS/2 Keyboard Driver
=====================

1. Overview
-----------

The PS/2 keyboard driver provides input from PS/2 keyboards. It supports
reading key presses and converting them to ASCII characters.

2. Configuration
----------------

The driver is optional and controlled by CONFIG_PS2_KBD:
  #ifdef CONFIG_PS2_KBD
      Full driver compiled in
  #else
      Stub functions (halts on read)

3. API
-----

  void kbd_init(void)
      - Initialize PS/2 keyboard

  int kbd_getchar(void)
      - Get a character (blocking)
      - Returns ASCII character code

  int kbd_getchar_nonblock(void)
      - Get a character (non-blocking)
      - Returns character or -1 if no key pressed

4. Implementation Details
--------------------------

The PS/2 keyboard interface uses:
  - I/O ports for keyboard communication
  - Scancode set 2 (most common)
  - Translation to ASCII using keymap

5. Key Mapping
--------------

The driver translates PS/2 scancodes to ASCII characters, handling:
  - Regular keys (a-z, 0-9, etc.)
  - Modifier keys (Shift, Ctrl, Alt)
  - Special keys (Enter, Backspace, Escape)

6. Buffer
---------

Key presses are typically buffered to allow non-blocking reads.
The driver maintains an internal circular buffer.
