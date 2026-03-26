Kernel Library Functions
=========================

1. Overview
-----------

The kernel library (src/lib/) provides common utility functions used
throughout the kernel.

2. String Functions (string.h / string.c)
----------------------------------------

  size_t strlen(const char *s)
      - Return length of string

  char *strcpy(char *dest, const char *src)
      - Copy string

  char *strncpy(char *dest, const char *src, size_t n)
      - Copy up to n characters

  int strcmp(const char *s1, const char *s2)
      - Compare strings

  int strncmp(const char *s1, const char *s2, size_t n)
      - Compare up to n characters

  void *memset(void *s, int c, size_t n)
      - Fill memory with byte

  void *memcpy(void *dest, const void *src, size_t n)
      - Copy memory

  void *memmove(void *dest, const void *src, size_t n)
      - Copy memory (handles overlap)

  int memcmp(const void *s1, const void *s2, size_t n)
      - Compare memory

3. Printf Implementation (printf.h / printf.c)
----------------------------------------------

The kernel includes a printf-style formatter:

  int printf(const char *fmt, ...)
      - Print formatted output

  int sprintf(char *buf, const char *fmt, ...)
      - Print to string

Supported format specifiers:
  %c - Character
  %s - String
  %d - Signed decimal
  %u - Unsigned decimal
  %x - Hexadecimal (lowercase)
  %X - Hexadecimal (uppercase)
  %p - Pointer
  %l - Long
  %ll - Long long

4. Math Functions (math.h / math.h)
------------------------------------

The kernel includes basic math functions for internal use.

5. Usage
--------

These library functions are used by:
  - Kernel logging (klog uses printf)
  - String handling in VFS
  - Memory operations in drivers
  - Various kernel subsystems
