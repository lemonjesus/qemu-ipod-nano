#include "hw/arm/ipod_nano3g_sysic.h"

static uint64_t ipod_nano3g_sysic_read(void *opaque, hwaddr addr, unsigned size)
{
    IPodNano3GSYSICState *s = (IPodNano3GSYSICState *) opaque;

    //fprintf(stderr, "%s: offset = 0x%08x\n", __func__, addr);

    switch (addr) {
        case POWER_ID:
            //return (3 << 24); //for older iboots
            return (2 << 0x18);
        case POWER_SETSTATE:
        case POWER_STATE:
            return s->power_state;
        case 0x7a:
        case 0x7c:
            return 1;
        case GPIO_INTLEVEL ... (GPIO_INTLEVEL + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTLEVEL) / 4;
            return s->gpio_int_level[group];
        }
        case GPIO_INTSTAT ... (GPIO_INTSTAT + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTSTAT) / 4;
            return s->gpio_int_status[group];
        }
        case GPIO_INTEN ... (GPIO_INTEN + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTEN) / 4;
            return s->gpio_int_enabled[group];
        }
        case GPIO_INTTYPE ... (GPIO_INTTYPE + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTTYPE) / 4;
            return s->gpio_int_type[group];
        }
      default:
        break;
    }
    return 0;
}

static void ipod_nano3g_sysic_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    IPodNano3GSYSICState *s = (IPodNano3GSYSICState *) opaque;

    //fprintf(stderr, "%s: writing 0x%08x to 0x%08x\n", __func__, val, addr);

    switch (addr) {
        case POWER_ONCTRL:
            if((val & 0x20) != 0 || (val & 0x4) != 0 || (val & POWER_ID_NAND) != 0) { break; } // make sure that we do not record the 'on' state of some devices so it appears like they are turned on immediately.
            s->power_state = val;
            break;
        case POWER_OFFCTRL:
            s->power_state = val;
            break;
        case GPIO_INTLEVEL ... (GPIO_INTLEVEL + GPIO_NUMINTGROUPS * 4):
        {
            break;
        }
        case GPIO_INTSTAT ... (GPIO_INTSTAT + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTSTAT) / 4;

            // acknowledge the interrupts and clear the corresponding bits
            s->gpio_int_status[group] = s->gpio_int_status[group] & ~val;

            qemu_irq_lower(s->gpio_irqs[group]);

            break;
        }
        case GPIO_INTEN ... (GPIO_INTEN + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTEN) / 4;
            s->gpio_int_enabled[group] = val;
            break;
        }
        case GPIO_INTTYPE ... (GPIO_INTTYPE + GPIO_NUMINTGROUPS * 4):
        {
            uint8_t group = (addr - GPIO_INTTYPE) / 4;
            s->gpio_int_type[group] = val;
            break;
        }
        default:
            break;
    }
}

static const MemoryRegionOps ipod_nano3g_sysic_ops = {
    .read = ipod_nano3g_sysic_read,
    .write = ipod_nano3g_sysic_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void ipod_nano3g_sysic_init(Object *obj)
{
    IPodNano3GSYSICState *s = IPOD_NANO3G_SYSIC(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &ipod_nano3g_sysic_ops, s, TYPE_IPOD_NANO3G_SYSIC, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    for(int grp = 0; grp < GPIO_NUMINTGROUPS; grp++) {
        sysbus_init_irq(sbd, &s->gpio_irqs[grp]);
    }
}

static void ipod_nano3g_sysic_class_init(ObjectClass *klass, void *data)
{
    
}

static const TypeInfo ipod_nano3g_sysic_type_info = {
    .name = TYPE_IPOD_NANO3G_SYSIC,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GSYSICState),
    .instance_init = ipod_nano3g_sysic_init,
    .class_init = ipod_nano3g_sysic_class_init,
};

static void ipod_nano3g_sysic_register_types(void)
{
    type_register_static(&ipod_nano3g_sysic_type_info);
}

type_init(ipod_nano3g_sysic_register_types)
