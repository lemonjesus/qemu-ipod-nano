#include "hw/arm/ipod_nano3g_clock.h"

static void S5L8702_clock1_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    IPodNano3GClockState *s = (struct IPodNano3GClockState *) opaque;

    switch (addr) {
        case CLOCK1_PLLLOCK:
            s->plllock = val;
            break;
      default:
            break;
    }
}

static uint64_t S5L8702_clock1_read(void *opaque, hwaddr addr, unsigned size)
{
    IPodNano3GClockState *s = (struct IPodNano3GClockState *) opaque;

    switch (addr) {
        case CLOCK1_CONFIG0:
            return s->config0;
        case CLOCK1_CONFIG1:
            return s->config1;
        case CLOCK1_CONFIG2:
            return s->config2;
        case CLOCK1_PLL0CON:
            return s->pll0con;
        case CLOCK1_PLL1CON:
            return s->pll1con;
        case CLOCK1_PLL2CON:
            return s->pll2con;
        case CLOCK1_PLL3CON:
            return s->pll3con;
        case CLOCK1_PLLLOCK:
            return s->plllock;
        case CLOCK1_PLLMODE:
            return 0x000a003a; // extracted from iPod Touch
      default:
            break;
    }
    return 0;
}

static const MemoryRegionOps clock1_ops = {
    .read = S5L8702_clock1_read,
    .write = S5L8702_clock1_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void S5L8702_clock_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    IPodNano3GClockState *s = IPOD_NANO3G_CLOCK(dev);

    s->plllock = 1 | 2 | 4 | 8;

    uint64_t config0 = 0x0;
    config0 |= (1 << 12); // set the clock PPL to index 1
    config0 |= (1 << 24); // indicate that we have a memory divisor
    config0 |= (2 << 16); // set the memory divisor to two
    s->config0 = config0;

    uint64_t config1 = 0x0;
    config1 |= (1 << 12); // set the bus PPL to index 1
    config1 |= (1 << 24); // indicate that we have a bus divisor
    config1 |= (3 << 16); // set the bus divisor to three
    config1 |= (1 << 8);  // indicate that unknown has a divisor
    config1 |= 3;         // set the unknown divisor 1 to 3
    config1 |= (0 << 4);  // set the unknown divisor 2 to 0
    config1 |= (1 << 20); // set the peripheral factor to 1

    config1 |= (1 << 14); // unknown
    config1 |= (1 << 28); // set some PPL index to 1
    config1 |= (1 << 30); // unknown
    s->config1 = config1;

    uint64_t config2 = 0x0;
    config2 |= (3 << 28); // set the peripheral PPL to index 3
    config2 |= (1 << 24); // indicate that the display has a divisor
    config2 |= (1 << 16); // set the display divisor to 1
    s->config2 = config2;

    // set the PLL configurations (MDIV, PDIV, SDIV)
    s->pll0con = (80 << 8) | (8 << 24) | 0;
    s->pll1con = (103 << 8) | (6 << 24) | 0;
    s->pll2con = (156 << 8) | (53 << 24) | 2;
    s->pll3con = (72 << 8) | (8 << 24) | 1;

    memory_region_init_io(&s->iomem, obj, &clock1_ops, s, "clock", 0x1000);
}

static void S5L8702_clock_class_init(ObjectClass *klass, void *data)
{

}

static const TypeInfo ipod_nano3g_clock_info = {
    .name          = TYPE_IPOD_NANO3G_CLOCK,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GClockState),
    .instance_init = S5L8702_clock_init,
    .class_init    = S5L8702_clock_class_init,
};

static void ipod_nano3g_machine_types(void)
{
    type_register_static(&ipod_nano3g_clock_info);
}

type_init(ipod_nano3g_machine_types)