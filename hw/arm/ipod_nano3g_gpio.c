#include "hw/arm/ipod_nano3g_gpio.h"

static void S5L8702_gpio_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    //fprintf(stderr, "%s: writing 0x%08x to 0x%08x\n", __func__, value, addr);
    IPodNano3GGPIOState *s = (struct IPodNano3GGPIOState *) opaque;

    switch(addr) {
      default:
        break;
    }
}

static uint64_t S5L8702_gpio_read(void *opaque, hwaddr addr, unsigned size)
{
    //fprintf(stderr, "%s: read from location 0x%08x\n", __func__, addr);
    IPodNano3GGPIOState *s = (struct IPodNano3GGPIOState *) opaque;

    switch(addr) {
        case 0x2c4:
            return s->gpio_state; 
        default:
            break;
    }

    return 0;
}

static const MemoryRegionOps gpio_ops = {
    .read = S5L8702_gpio_read,
    .write = S5L8702_gpio_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void S5L8702_gpio_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    IPodNano3GGPIOState *s = IPOD_NANO3G_GPIO(dev);

    memory_region_init_io(&s->iomem, obj, &gpio_ops, s, "gpio", 0x10000);
}

static void S5L8702_gpio_class_init(ObjectClass *klass, void *data)
{

}

static const TypeInfo ipod_nano3g_gpio_info = {
    .name          = TYPE_IPOD_NANO3G_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GGPIOState),
    .instance_init = S5L8702_gpio_init,
    .class_init    = S5L8702_gpio_class_init,
};

static void ipod_nano3g_machine_types(void)
{
    type_register_static(&ipod_nano3g_gpio_info);
}

type_init(ipod_nano3g_machine_types)