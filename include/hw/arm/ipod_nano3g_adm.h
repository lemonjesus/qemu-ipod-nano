#ifndef IPOD_NANO3G_ADM_H
#define IPOD_NANO3G_ADM_H

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/arm/ipod_nano3g_nand.h"

#define TYPE_IPOD_NANO3G_ADM                "ipodnano3g.adm"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GADMState, IPOD_NANO3G_ADM)

#define ADM_CTRL 0x0
#define ADM_CTRL2 0x4
#define ADM_CODE_SEC_ADDR 0x50
#define ADM_DATA1_SEC_ADDR 0x84
#define ADM_DATA2_SEC_ADDR 0x88
#define ADM_DATA3_SEC_ADDR 0x8C

typedef struct IPodNano3GADMState {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t code_sec_addr;
    uint32_t data1_sec_addr;
    uint32_t data2_sec_addr;
    uint32_t data3_sec_addr;

    MemoryRegion *downstream;
    AddressSpace downstream_as;

    NandState *nand_state;
} IPodNano3GADMState;

#endif