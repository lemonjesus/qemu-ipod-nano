#ifndef IPOD_NANO3G_TVOUT_H
#define IPOD_NANO3G_TVOUT_H

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"
#include "hw/irq.h"

#define SDO_IRQ 0x280

#define TYPE_IPOD_NANO3G_TVOUT                "ipodnano3g.tvout"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GTVOutState, IPOD_NANO3G_TVOUT)

typedef struct IPodNano3GTVOutState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;
    uint32_t index;

    //uint32_t sdo_irq_reg;
    uint32_t data[4096];
} IPodNano3GTVOutState;

#endif