#include "hw/arm/ipod_nano3g_sdio.h"

void sdio_exec_cmd(IPodNano3GSDIOState *s)
{
    uint32_t addr = (s->arg >> 9) & 0x1ffff;
    if(addr == (1 << 8)) {
        // reading slot 1 - make sure there is a device here
        //s->resp0 = 1;
    }
}

static void ipod_nano3g_sdio_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    fprintf(stderr, "%s: writing 0x%08x to 0x%08x\n", __func__, value, addr);
    
    IPodNano3GSDIOState *s = (struct IPodNano3GSDIOState *) opaque;

    switch(addr) {
        case SDIO_CMD:
            s->cmd = value;
            if(value & (1 << 31)) {
                sdio_exec_cmd(s);
            }
            break;
        case SDIO_ARGU:
            s->arg = value;
            break;
        case SDIO_CSR:
            s->csr = value;
            break;
        case SDIO_IRQMASK:
            s->irq_mask = value;
            break;
        default:
            break;
    }
}

static uint64_t ipod_nano3g_sdio_read(void *opaque, hwaddr addr, unsigned size)
{
    fprintf(stderr, "%s: offset = 0x%08x\n", __func__, addr);

    IPodNano3GSDIOState *s = (struct IPodNano3GSDIOState *) opaque;

    switch (addr) {
        case SDIO_CMD:
            return s->cmd;
        case SDIO_ARGU:
            return s->arg;
        case SDIO_DSTA:
            return (1 << 0) | (1 << 4) ; // 0x1 indicates that the SDIO is ready for a CMD, (1 << 4) that the command is complete
        case SDIO_RESP0:
            return s->resp0;
        case SDIO_RESP1:
            return s->resp1;
        case SDIO_RESP2:
            return s->resp2;
        case SDIO_RESP3:
            return s->resp3;
        case SDIO_CSR:
            return s->csr;
        case SDIO_IRQMASK:
            return s->irq_mask;
        default:
            break;
    }

    return 0;
}

static const MemoryRegionOps ipod_nano3g_sdio_ops = {
    .read = ipod_nano3g_sdio_read,
    .write = ipod_nano3g_sdio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void ipod_nano3g_sdio_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    IPodNano3GSDIOState *s = IPOD_NANO3G_SDIO(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &ipod_nano3g_sdio_ops, s, TYPE_IPOD_NANO3G_SDIO, 4096);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void ipod_nano3g_sdio_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
}

static const TypeInfo ipod_nano3g_sdio_type_info = {
    .name = TYPE_IPOD_NANO3G_SDIO,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GSDIOState),
    .instance_init = ipod_nano3g_sdio_init,
    .class_init = ipod_nano3g_sdio_class_init,
};

static void ipod_nano3g_sdio_register_types(void)
{
    type_register_static(&ipod_nano3g_sdio_type_info);
}

type_init(ipod_nano3g_sdio_register_types)
