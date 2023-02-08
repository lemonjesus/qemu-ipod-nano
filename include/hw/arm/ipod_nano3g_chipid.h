#ifndef HW_ARM_IPOD_NANO3G_CHIPID_H
#define HW_ARM_IPOD_NANO3G_CHIPID_H

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"

#define TYPE_IPOD_NANO3G_CHIPID "ipodnano3g.chipid"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GChipIDState, IPOD_NANO3G_CHIPID)

#define CHIP_REVISION 0x0

typedef struct IPodNano3GChipIDState {
    SysBusDevice busdev;
    MemoryRegion iomem;
} IPodNano3GChipIDState;

#endif