#ifndef HW_ARM_IPOD_TOUCH_CHIPID_H
#define HW_ARM_IPOD_TOUCH_CHIPID_H

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/sysbus.h"

#define TYPE_IPOD_TOUCH_CHIPID "ipodtouch.chipid"
OBJECT_DECLARE_SIMPLE_TYPE(IPodTouchChipIDState, IPOD_TOUCH_CHIPID)

#define CHIP_REVISION 0x0

typedef struct IPodTouchChipIDState {
    SysBusDevice busdev;
    MemoryRegion iomem;
} IPodTouchChipIDState;

#endif