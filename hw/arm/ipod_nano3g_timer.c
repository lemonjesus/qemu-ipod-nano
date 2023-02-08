#include "hw/arm/ipod_nano3g_timer.h"

static void S5L8702_st_update(IPodNano3GTimerState *s)
{
    s->freq_out = 1000000000 / 100; 
    s->tick_interval = /* bcount1 * get_ticks / freq  + ((bcount2 * get_ticks / freq)*/
    muldiv64((s->bcount1 < 1000) ? 1000 : s->bcount1, NANOSECONDS_PER_SECOND, s->freq_out);
    s->next_planned_tick = 0;
}

static void S5L8702_st_set_timer(IPodNano3GTimerState *s)
{
    uint64_t last = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) - s->base_time;

    s->next_planned_tick = last + (s->tick_interval - last % s->tick_interval);
    timer_mod(s->st_timer, s->next_planned_tick + s->base_time);
    s->last_tick = last;
}

static void S5L8702_st_tick(void *opaque)
{
    IPodNano3GTimerState *s = (IPodNano3GTimerState *)opaque;

    if (s->status & TIMER_STATE_START) {
        //fprintf(stderr, "%s: Raising irq\n", __func__);
        qemu_irq_raise(s->irq);

        /* schedule next interrupt */
        if(!(s->status & TIMER_STATE_MANUALUPDATE)) {
            S5L8702_st_set_timer(s);
        }
    } else {
        s->next_planned_tick = 0;
        s->last_tick = 0;
        timer_del(s->st_timer);
    }
}

static void S5L8702_timer1_write(void *opaque, hwaddr addr, uint64_t value, unsigned size)
{
    //fprintf(stderr, "%s: writing 0x%08x to 0x%08x\n", __func__, value, addr);
    IPodNano3GTimerState *s = (struct IPodNano3GTimerState *) opaque;

    switch(addr){

        case TIMER_IRQSTAT:
            s->irqstat = value;
            return;
        case TIMER_IRQLATCH:
            //fprintf(stderr, "%s: lowering irq\n", __func__);
            qemu_irq_lower(s->irq);     
            return;
        case TIMER_4 + TIMER_CONFIG:
            S5L8702_st_update(s);
            s->config = value;
            break;
        case TIMER_4 + TIMER_STATE:
            if (value & TIMER_STATE_START) {
                s->base_time = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
                S5L8702_st_update(s);
                S5L8702_st_set_timer(s);
            } else if (value == TIMER_STATE_STOP) {
                timer_del(s->st_timer);
            }
            s->status = value;
            break;
        case TIMER_4 + TIMER_COUNT_BUFFER:
            s->bcount1 = s->bcreload = value;
            break;
        case TIMER_4 + TIMER_COUNT_BUFFER2:
            s->bcount2 = value;
            break;
      default:
        break;
    }
}

static uint64_t S5L8702_timer1_read(void *opaque, hwaddr addr, unsigned size)
{
    // fprintf(stderr, "%s: read from location 0x%08x\n", __func__, addr);
    IPodNano3GTimerState *s = (struct IPodNano3GTimerState *) opaque;
    uint64_t elapsed_ns, ticks;

    switch (addr) {
        case TIMER_TICKSLOW:    // needs to be fixed so that read from low first works as well

            elapsed_ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) / 2; // the timer ticks twice as slow as the CPU frequency in the kernel
            ticks = clock_ns_to_ticks(s->sysclk, elapsed_ns);
            // printf("TICKS: %lld\n", ticks);
            s->ticks_high = (ticks >> 32);
            s->ticks_low = (ticks & 0xFFFFFFFF);
            return s->ticks_low;
        // case TIMER_TICKSLOW:
        //     return s->ticks_low;
        case TIMER_IRQSTAT:
            return ~0; // s->irqstat;
        case TIMER_IRQLATCH:
            return 0xffffffff;

      default:
        break;
    }
    return 0;
}

static const MemoryRegionOps timer1_ops = {
    .read = S5L8702_timer1_read,
    .write = S5L8702_timer1_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void S5L8702_timer_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    IPodNano3GTimerState *s = IPOD_NANO3G_TIMER(dev);

    memory_region_init_io(&s->iomem, obj, &timer1_ops, s, "timer1", 0x10001);
    sysbus_init_irq(sbd, &s->irq);

    s->base_time = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    s->st_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, S5L8702_st_tick, s);
}

static void S5L8702_timer_class_init(ObjectClass *klass, void *data)
{

}

static const TypeInfo ipod_nano3g_timer_info = {
    .name          = TYPE_IPOD_NANO3G_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GTimerState),
    .instance_init = S5L8702_timer_init,
    .class_init    = S5L8702_timer_class_init,
};

static void ipod_nano3g_machine_types(void)
{
    type_register_static(&ipod_nano3g_timer_info);
}

type_init(ipod_nano3g_machine_types)