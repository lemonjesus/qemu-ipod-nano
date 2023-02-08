#ifndef IPOD_NANO3G_SDIO_H
#define IPOD_NANO3G_SDIO_H

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"

#define TYPE_IPOD_NANO3G_SDIO                "ipodnano3g.sdio"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GSDIOState, IPOD_NANO3G_SDIO)

#define SDIO_CMD        0x8
#define SDIO_ARGU       0xC
#define SDIO_DSTA       0x18
#define SDIO_RESP0      0x20
#define SDIO_RESP1      0x24
#define SDIO_RESP2      0x28
#define SDIO_RESP3      0x2C
#define SDIO_CSR        0x34
#define SDIO_IRQMASK    0x3C

typedef struct IPodNano3GSDIOState
{
    SysBusDevice parent_obj;
    MemoryRegion iomem;

    uint32_t cmd;
    uint32_t arg;
    uint32_t csr;
    uint32_t resp0;
    uint32_t resp1;
    uint32_t resp2;
    uint32_t resp3;
    uint32_t irq_mask;
} IPodNano3GSDIOState;

#endif