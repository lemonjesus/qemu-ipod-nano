#include "hw/arm/ipod_nano3g_tvout.h"
#include "qapi/error.h"

static uint64_t ipod_nano3g_tvout_read(void *opaque, hwaddr offset, unsigned size)
{
    IPodNano3GTVOutState *s = (IPodNano3GTVOutState *)opaque;

    //fprintf(stderr, "%s (%d): offset = 0x%08x\n", __func__, s->index, offset);

    // switch(offset) {
    //     case SDO_IRQ:
    //         return s->sdo_irq_reg;
    // }

    return s->data[offset];
}

static void ipod_nano3g_tvout_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    IPodNano3GTVOutState *s = (IPodNano3GTVOutState *)opaque;

    //fprintf(stderr, "%s (%d): writing 0x%08x to 0x%08x\n", __func__, s->index, value, offset);
    s->data[offset] = value;
    // switch(offset) {
    //     case SDO_IRQ:
    //         s->sdo_irq_reg = value;
    //         break;
    //     default:
    //         break;
    // }
}

static const MemoryRegionOps ipod_nano3g_tvout_ops = {
    .read = ipod_nano3g_tvout_read,
    .write = ipod_nano3g_tvout_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void ipod_nano3g_tvout_init(Object *obj)
{
    DeviceState *dev = DEVICE(obj);
    IPodNano3GTVOutState *s = IPOD_NANO3G_TVOUT(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &ipod_nano3g_tvout_ops, s, "tvout", 4096);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void ipod_nano3g_tvout_class_init(ObjectClass *klass, void *data)
{

}

static const TypeInfo ipod_nano3g_tvout_type_info = {
    .name = TYPE_IPOD_NANO3G_TVOUT,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GTVOutState),
    .instance_init = ipod_nano3g_tvout_init,
    .class_init = ipod_nano3g_tvout_class_init,
};

static void ipod_nano3g_tvout_register_types(void)
{
    type_register_static(&ipod_nano3g_tvout_type_info);
}

type_init(ipod_nano3g_tvout_register_types)
