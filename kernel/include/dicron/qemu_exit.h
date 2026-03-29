
#ifndef _DICRON_QEMU_EXIT_H
#define _DICRON_QEMU_EXIT_H

#include <stdint.h>
#include "arch/x86_64/io.h"

/*
 * QEMU ISA debug-exit device
 *
 * Enabled in QEMU with: -device isa-debug-exit,iobase=0xf4,iosize=0x04
 *
 * Writing value V to port 0xf4 causes QEMU to exit with code (V << 1) | 1.
 * We use V = 0x10 so the host exit code is (0x10 << 1) | 1 = 33.
 * The CI step checks for exit code 33 to mean SUCCESS.
 */
#define QEMU_EXIT_PORT		0xf4
#define QEMU_EXIT_SUCCESS_VAL	0x10	/* host sees (0x10<<1)|1 = 33 */
#define QEMU_EXIT_FAILURE_VAL	0x00	/* host sees (0x00<<1)|1 = 1  */

static inline void qemu_exit(uint32_t code)
{
	outl(QEMU_EXIT_PORT, code);
}

#endif /* _DICRON_QEMU_EXIT_H */
