#include "hw/arm/ipod_nano3g_chipid.h"

static uint64_t ipod_nano3g_chipid_read(void *opaque, hwaddr addr, unsigned size)
{
    //fprintf(stderr, "%s: offset = 0x%08x\n", __func__, addr);

    switch (addr) {
        case 0x0:
            return (CHIP_REVISION << 24);
        default:
            break;
    }

    return 0;
}

static const MemoryRegionOps ipod_nano3g_chipid_ops = {
    .read = ipod_nano3g_chipid_read,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void ipod_nano3g_chipid_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    IPodNano3GChipIDState *s = IPOD_NANO3G_CHIPID(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &ipod_nano3g_chipid_ops, s, TYPE_IPOD_NANO3G_CHIPID, 0x10);
    sysbus_init_mmio(sbd, &s->iomem);
}

static void ipod_nano3g_chipid_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
}

static const TypeInfo ipod_nano3g_chipid_type_info = {
    .name = TYPE_IPOD_NANO3G_CHIPID,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GChipIDState),
    .instance_init = ipod_nano3g_chipid_init,
    .class_init = ipod_nano3g_chipid_class_init,
};

static void ipod_nano3g_chipid_register_types(void)
{
    type_register_static(&ipod_nano3g_chipid_type_info);
}

type_init(ipod_nano3g_chipid_register_types)
