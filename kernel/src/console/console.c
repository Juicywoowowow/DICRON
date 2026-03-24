#include "console.h"
#include "font.h"
#include "lib/string.h"
#include "limine.h"
#include <dicron/mem.h>

static uint32_t *framebuffer;
static uint32_t *backbuffer;
static uint32_t  fb_width;
static uint32_t  fb_height;
static uint32_t  fb_pitch; /* bytes per scanline */

static uint32_t  cols;
static uint32_t  rows;
static uint32_t  cx; /* cursor col */
static uint32_t  cy; /* cursor row */

static uint32_t  fg_color = 0x00CCCCCC;
static uint32_t  bg_color = 0x00000000;

static uint32_t  dirty_y_min = (uint32_t)-1;
static uint32_t  dirty_y_max = 0;

static void mark_dirty(uint32_t row)
{
	if (row < dirty_y_min) dirty_y_min = row;
	if (row > dirty_y_max) dirty_y_max = row;
}

void console_flush(void)
{
	if (!backbuffer || dirty_y_min > dirty_y_max)
		return;

	uint32_t y0 = dirty_y_min * FONT_HEIGHT;
	uint32_t y1 = (dirty_y_max + 1) * FONT_HEIGHT;
	if (y1 > fb_height) y1 = fb_height;

	size_t offset = (size_t)y0 * (fb_pitch / 4);
	size_t length = (size_t)(y1 - y0) * fb_pitch;

	/* Sequential write straight to the uncacheable Framebuffer */
	memcpy((uint8_t *)framebuffer + (offset * 4),
	       (uint8_t *)backbuffer + (offset * 4), length);

	dirty_y_min = (uint32_t)-1;
	dirty_y_max = 0;
}

static void draw_char(uint32_t col, uint32_t row, char c)
{
	if (c < FONT_FIRST_CHAR || c > FONT_LAST_CHAR)
		c = '?';

	const uint8_t *glyph = font_data[(size_t)(c - FONT_FIRST_CHAR)];
	uint32_t x0 = col * FONT_WIDTH;
	uint32_t y0 = row * FONT_HEIGHT;

	uint32_t *target = backbuffer ? backbuffer : framebuffer;

	for (uint32_t y = 0; y < FONT_HEIGHT; y++) {
		uint32_t *pixel = (uint32_t *)((uint8_t *)target +
				  (y0 + y) * fb_pitch) + x0;
		for (uint32_t x = 0; x < FONT_WIDTH; x++)
			pixel[x] = (glyph[y] & (0x80 >> x)) ? fg_color : bg_color;
	}
	mark_dirty(row);
}

static void scroll(void)
{
	size_t row_bytes = (size_t)fb_pitch * FONT_HEIGHT;
	uint32_t *target = backbuffer ? backbuffer : framebuffer;

	memmove(target,
		(uint8_t *)target + row_bytes,
		row_bytes * (rows - 1));

	memset((uint8_t *)target + row_bytes * (rows - 1),
	       0, row_bytes);

	dirty_y_min = 0;
	dirty_y_max = rows - 1; /* Whole screen changed */
}

static void newline(void)
{
	cx = 0;
	cy++;
	if (cy >= rows) {
		cy = rows - 1;
		scroll();
	}
}

void console_init(struct limine_framebuffer *fb)
{
	framebuffer = (uint32_t *)fb->address;
	fb_width    = (uint32_t)fb->width;
	fb_height   = (uint32_t)fb->height;
	fb_pitch    = (uint32_t)fb->pitch;

	cols = fb_width / FONT_WIDTH;
	rows = fb_height / FONT_HEIGHT;
	cx   = 0;
	cy   = 0;

	/* Allocate backbuffer from kheap. Requires MAX_ORDER raised to 12. */
	backbuffer = kmalloc((size_t)fb_height * fb_pitch);

	console_clear();
	console_flush();
}

void console_putchar(char c)
{
	if (c == '\n') {
		newline();
		return;
	}
	if (c == '\r') {
		cx = 0;
		return;
	}
	if (c == '\t') {
		cx = (cx + 8) & ~7u;
		if (cx >= cols)
			newline();
		return;
	}
	if (c == '\b') {
		if (cx > 0)
			cx--;
		draw_char(cx, cy, ' ');
		return;
	}

	draw_char(cx, cy, c);
	cx++;
	if (cx >= cols)
		newline();
}

void console_write(const char *s, size_t len)
{
	for (size_t i = 0; i < len; i++)
		console_putchar(s[i]);
	console_flush();
}

void console_clear(void)
{
	uint32_t *target = backbuffer ? backbuffer : framebuffer;
	memset(target, 0, (size_t)fb_pitch * fb_height);
	cx = 0;
	cy = 0;
	dirty_y_min = 0;
	dirty_y_max = rows - 1;
	console_flush();
}
