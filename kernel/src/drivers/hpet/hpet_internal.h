#ifndef _DICRON_DRIVERS_HPET_INTERNAL_H
#define _DICRON_DRIVERS_HPET_INTERNAL_H

#include <stdint.h>

#define HPET_REG_CAPABILITIES   0x00
#define HPET_REG_CONFIG         0x10
#define HPET_REG_ISR            0x20
#define HPET_REG_MAIN_COUNTER   0xF0
#define HPET_REG_TIMER_BASE     0x100

extern volatile uint8_t *hpet_base;

#endif
