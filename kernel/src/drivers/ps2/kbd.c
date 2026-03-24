#include "kbd.h"
#include "drivers/registry.h"
#include "arch/x86_64/idt.h"
#include "arch/x86_64/io.h"
#include <dicron/dev.h>
#include <dicron/spinlock.h>
#include <stddef.h>
#include <stdint.h>

/* scancode set 1 key codes */
#define SC_LSHIFT_PRESS		0x2A
#define SC_LSHIFT_RELEASE	0xAA
#define SC_RSHIFT_PRESS		0x36
#define SC_RSHIFT_RELEASE	0xB6
#define SC_CAPSLOCK		0x3A
#define SC_CTRL_PRESS		0x1D
#define SC_CTRL_RELEASE		0x9D
#define SC_ALT_PRESS		0x38
#define SC_ALT_RELEASE		0xB8

/* US QWERTY scancode → ASCII (unshifted) */
static const char sc_lower[128] = {
	0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
	'\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
	0, /* ctrl */
	'a','s','d','f','g','h','j','k','l',';','\'','`',
	0, /* lshift */
	'\\','z','x','c','v','b','n','m',',','.','/',
	0, /* rshift */
	'*',
	0, /* alt */
	' ',
	0, /* caps */
	0,0,0,0,0,0,0,0,0,0, /* F1-F10 */
	0, /* numlock */
	0, /* scrolllock */
	0, /* home */
	0, /* up */
	0, /* pgup */
	'-',
	0, /* left */
	0,
	0, /* right */
	'+',
	0, /* end */
	0, /* down */
	0, /* pgdn */
	0, /* insert */
	0, /* delete */
};

/* US QWERTY scancode → ASCII (shifted) */
static const char sc_upper[128] = {
	0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
	'\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
	0, /* ctrl */
	'A','S','D','F','G','H','J','K','L',':','"','~',
	0, /* lshift */
	'|','Z','X','C','V','B','N','M','<','>','?',
	0, /* rshift */
	'*',
	0, /* alt */
	' ',
};

static volatile int shift_held;
static volatile int caps_on;
static volatile int ctrl_held;

#define KBD_BUF_SIZE 256

static volatile char   kbd_buf[KBD_BUF_SIZE];
static volatile size_t kbd_head;
static volatile size_t kbd_tail;
static spinlock_t      kbd_lock = SPINLOCK_INIT;

static void kbd_irq(struct idt_frame *frame)
{
	(void)frame;
	uint8_t sc = inb(0x60);

	/* modifier tracking */
	switch (sc) {
	case SC_LSHIFT_PRESS:
	case SC_RSHIFT_PRESS:
		shift_held = 1;
		return;
	case SC_LSHIFT_RELEASE:
	case SC_RSHIFT_RELEASE:
		shift_held = 0;
		return;
	case SC_CTRL_PRESS:
		ctrl_held = 1;
		return;
	case SC_CTRL_RELEASE:
		ctrl_held = 0;
		return;
	case SC_ALT_PRESS:
	case SC_ALT_RELEASE:
		return;
	case SC_CAPSLOCK:
		caps_on = !caps_on;
		return;
	}

	/* ignore key-up */
	if (sc & 0x80)
		return;

	char c = 0;
	if (sc < sizeof(sc_lower)) {
		int use_upper = shift_held;

		/* caps lock flips letter case only */
		c = sc_lower[sc];
		if (c >= 'a' && c <= 'z')
			use_upper = shift_held ^ caps_on;

		if (use_upper && sc < sizeof(sc_upper) && sc_upper[sc])
			c = sc_upper[sc];
	}

	if (c) {
		/*
		 * We're in IRQ context — interrupts already disabled.
		 * Use plain spin_lock (not irqsave).
		 */
		spin_lock(&kbd_lock);

		size_t next = (kbd_head + 1) % KBD_BUF_SIZE;
		if (next != kbd_tail) {
			kbd_buf[kbd_head] = c;
			kbd_head = next;
		}

		spin_unlock(&kbd_lock);
	}
}

int kbd_getchar_nonblock(void)
{
	uint64_t flags = spin_lock_irqsave(&kbd_lock);
	if (kbd_tail == kbd_head) {
		spin_unlock_irqrestore(&kbd_lock, flags);
		return -1;
	}
	char c = kbd_buf[kbd_tail];
	kbd_tail = (kbd_tail + 1) % KBD_BUF_SIZE;
	spin_unlock_irqrestore(&kbd_lock, flags);
	return c;
}

int kbd_getchar(void)
{
	for (;;) {
		int c = kbd_getchar_nonblock();
		if (c != -1)
			return c;
		__asm__ volatile("pause" ::: "memory");
	}
}

static ssize_t kbd_read(struct kdev *dev, void *buf, size_t len)
{
	(void)dev;
	char *p = buf;

	for (size_t i = 0; i < len; i++)
		p[i] = (char)kbd_getchar();
	return (ssize_t)len;
}

static struct kdev_ops kbd_ops = {
	.open  = NULL,
	.close = NULL,
	.read  = kbd_read,
	.write = NULL,
	.ioctl = NULL,
};

static struct kdev kbd_dev = {
	.name  = "kbd0",
	.minor = DRV_KBD,
	.type  = KDEV_CHAR,
	.ops   = &kbd_ops,
	.priv  = NULL,
};

void kbd_init(void)
{
	kbd_head = 0;
	kbd_tail = 0;
	shift_held = 0;
	caps_on = 0;
	ctrl_held = 0;

	idt_set_handler(0x21, kbd_irq);

	int err = drv_register(DRV_KBD, &kbd_dev);
	if (err)
		return;
}
