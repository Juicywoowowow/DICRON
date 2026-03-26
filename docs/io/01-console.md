Framebuffer Console
====================

1. Overview
-----------

The console driver provides text output on a graphics framebuffer.
It writes characters to video memory in a text mode configuration.

2. Configuration
----------------

The driver is optional and controlled by CONFIG_FRAMEBUFFER:
  #ifdef CONFIG_FRAMEBUFFER
      Full driver compiled in
  #else
      Stub functions (no-ops)

3. API
-----

  void console_init(struct limine_framebuffer *fb)
      - Initialize console with framebuffer info

  void console_putchar(char c)
      - Output a single character

  void console_write(const char *s, size_t len)
      - Output a string

  void console_clear(void)
      - Clear the screen

  void console_flush(void)
      - Flush output buffer

4. Framebuffer Information
---------------------------

The framebuffer is provided by Limine bootloader:
  - Physical address
  - Width and height (pixels)
  - Bits per pixel
  - Pitch (bytes per row)
  - Format (RGB or BGR)

5. Text Rendering
-----------------

The console uses an 8x16 pixel font (defined in font_data.c):
  - 16 rows of 8x16 characters fit in typical 80x25 display
  - Character bitmap data stored as 16 bytes per character
  - Color: White on black background

6. Implementation Details
-------------------------

  - Maintains cursor position (row, column)
  - Handles newline and scroll behavior
  - Writes directly to framebuffer memory
  - Supports backspace and basic control characters

7. Buffering
-----------

Output may be buffered for performance:
  - console_putchar adds to buffer
  - console_flush writes to framebuffer

8. Limine Integration
---------------------

The framebuffer is obtained from the Limine framebuffer request
during kernel initialization.
