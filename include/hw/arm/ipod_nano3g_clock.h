#ifndef IPOD_NANO3G_CLOCK_H
#define IPOD_NANO3G_CLOCK_H

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/clock.h"

#define TYPE_IPOD_NANO3G_CLOCK                "ipodnano3g.clock"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GClockState, IPOD_NANO3G_CLOCK)

#define CLOCK1_CONFIG0 0x0
#define CLOCK1_CONFIG1 0x4
#define CLOCK1_CONFIG2 0x8
#define CLOCK1_PLL0CON 0x20
#define CLOCK1_PLL1CON 0x24
#define CLOCK1_PLL2CON 0x28
#define CLOCK1_PLL3CON 0x2C
#define CLOCK1_PLLLOCK 0x40
#define CLOCK1_PLLMODE 0x44

typedef struct IPodNano3GClockState
{
    SysBusDevice busdev;
    MemoryRegion iomem;
    uint32_t    config0;
    uint32_t    config1;
    uint32_t    config2;
    uint32_t    pll0con;
    uint32_t    pll1con;
    uint32_t    pll2con;
    uint32_t    pll3con;
    uint32_t    plllock;

} IPodNano3GClockState;

#endif