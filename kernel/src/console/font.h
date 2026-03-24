#ifndef _DICRON_CONSOLE_FONT_H
#define _DICRON_CONSOLE_FONT_H

#include <stdint.h>

#define FONT_WIDTH  8
#define FONT_HEIGHT 16
#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR  126

/* 8x16 bitmap font — ASCII 32-126 */
extern const uint8_t font_data[95][16];

#endif
