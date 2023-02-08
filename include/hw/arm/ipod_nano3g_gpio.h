#ifndef IPOD_NANO3G_GPIO_H
#define IPOD_NANO3G_GPIO_H

#include <math.h>
#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"

#define TYPE_IPOD_NANO3G_GPIO                "ipodnano3g.gpio"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GGPIOState, IPOD_NANO3G_GPIO)

#define GPIO_BUTTON_POWER 0x1605
#define GPIO_BUTTON_HOME  0x1606

#define GPIO_BUTTON_POWER_IRQ 0x2D
#define GPIO_BUTTON_HOME_IRQ  0x2E

#define NUM_GPIO_PINS 0x20

typedef struct IPodNano3GGPIOState
{
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    uint32_t gpio_state;
} IPodNano3GGPIOState;

#endif