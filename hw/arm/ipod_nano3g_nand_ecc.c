#include "hw/arm/ipod_nano3g_nand_ecc.h"

static uint64_t nand_ecc_read(void *opaque, hwaddr addr, unsigned size)
{
    NandECCState *s = (NandECCState *) opaque;
    //fprintf(stderr, "%s: reading from 0x%08x\n", __func__, addr);

    switch (addr) {
        case NANDECC_STATUS:
            return 0; // TODO: for now, we assume that all ECC operations are successful
        default:
            break;
    }
    return 0;
}

static void nand_ecc_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    NandECCState *s = (NandECCState *) opaque;

    switch(addr) {
        case NANDECC_START:
            qemu_irq_raise(s->irq);
            break;
        case NANDECC_CLEARINT:
            qemu_irq_lower(s->irq);
            break;
        default:
            break;
    }
}

static const MemoryRegionOps nand_ecc_ops = {
    .read = nand_ecc_read,
    .write = nand_ecc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void nand_ecc_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    NandECCState *s = IPOD_NANO3G_NANDECC(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &nand_ecc_ops, s, "nandecc", 0x100);
    sysbus_init_irq(sbd, &s->irq);
}

static void nand_ecc_reset(DeviceState *d)
{
    NandECCState *s = (NandECCState *) d;

    s->data_addr = 0;
    s->ecc_addr = 0;
    s->status = 0;
    s->setup = 0;
}

static void nand_ecc_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->reset = nand_ecc_reset;
}

static const TypeInfo nand_ecc_info = {
    .name          = TYPE_IPOD_NANO3G_NANDECC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NandECCState),
    .instance_init = nand_ecc_init,
    .class_init    = nand_ecc_class_init,
};

static void nand_ecc_register_types(void)
{
    type_register_static(&nand_ecc_info);
}

type_init(nand_ecc_register_types)
