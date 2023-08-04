#include "hw/arm/ipod_nano3g_gpio.h"

#define ENABLE_CLICKWHEEL_DEBUG 0
#define CLICKWHEEL_DEBUG if (ENABLE_CLICKWHEEL_DEBUG) printf

static void S5L8702_gpio_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) {
    // CLICKWHEEL_DEBUG("%s: writing 0x%08x to 0x%08x\n", __func__, value, addr);
    IPodNano3GGPIOState *s = (struct IPodNano3GGPIOState *) opaque;

    switch(addr) {
        case GPIO_CMD:
            uint8_t set = (value & 0x00FF0000) >> 16;
            uint8_t pin = (value & 0x0000FF00) >> 8;
            uint8_t state = (value & 0x000000FF);

            s->gpio_pin_state[set][pin] = state;
            CLICKWHEEL_DEBUG("GPIO: set %x, pin %x, state %x\n", set, pin, state);
            break;
      default:
        break;
    }
}

static uint64_t S5L8702_gpio_read(void *opaque, hwaddr addr, unsigned size) {
    // CLICKWHEEL_DEBUG("%s: read from location 0x%08x\n", __func__, addr);
    IPodNano3GGPIOState *s = (struct IPodNano3GGPIOState *) opaque;

    switch(addr) {
        case GPIO_PDATE:
            // this is the clickwheel code. brace yourself.
            // GPIOe.2 indicates who is talking: high = clickwheel, low = iPod
            // GPIOe.3 is the clock, likely driven by the clickwheel, data latch on rising edge
            // GPIOe.4 TX to clickwheel
            // GPIOe.5 RX from clickwheel

            uint8_t out = 0x00;
            uint64_t ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

            if(s->clickwheel_clk) {
                // CLICKWHEEL_DEBUG("clickwheel falling edge\n");
                s->clickwheel_clk = 0;
                if(s->gpio_pin_state[0xe][2] & 0x01) {
                    s->clickwheel_bit_to_send = s->clickwheel_tx_buf & 1;
                    s->clickwheel_tx_buf >>= 1;
                    out |= s->clickwheel_bit_to_send << 5;
                    s->clickwheel_skip_cycle = 1;
                    CLICKWHEEL_DEBUG("clickwheel is sending %d, tx_buf = 0x%08x, %ld\n", s->clickwheel_bit_to_send, s->clickwheel_tx_buf, ns);
                }
            } else {
                // CLICKWHEEL_DEBUG("clickwheel rising edge\n");
                if(s->clickwheel_skip_cycle) {
                    s->clickwheel_skip_cycle = 0;
                    CLICKWHEEL_DEBUG("skipping clickwheel rising edge\n");
                } else {
                    s->clickwheel_clk = 1;
                    out |= 0b00001000;
                }

                if(s->gpio_pin_state[0xe][2] & 0x01) {
                    out |= s->clickwheel_bit_to_send << 5;
                    CLICKWHEEL_DEBUG("clickwheel is sending %d, tx_buf = 0x%08x, %ld\n", s->clickwheel_bit_to_send, s->clickwheel_tx_buf, ns);
                } else {
                    // clickwheel is receiving, clock in data and check if we have a full command
                    s->clickwheel_rx_buf <<= 1;
                    s->clickwheel_rx_buf |= s->gpio_pin_state[0xe][4] & 0x01;
                    CLICKWHEEL_DEBUG("clickwheel is receiving, rx_buf = 0x%08x, %ld\n", s->clickwheel_rx_buf, ns);
                    switch(s->clickwheel_rx_buf) {
                        case 0xb8800003: // reverse of c000011d: read button presses
                            s->clickwheel_tx_buf = 0x8000023a;
                            
                            if(s->clickwheel_select_pressed) s->clickwheel_tx_buf |= (1 << 0x10);
                            if(s->clickwheel_play_pressed) s->clickwheel_tx_buf |= (1 << 0x11);
                            if(s->clickwheel_prev_pressed) s->clickwheel_tx_buf |= (1 << 0x12);
                            if(s->clickwheel_menu_pressed) s->clickwheel_tx_buf |= (1 << 0x13);
                            if(s->clickwheel_next_pressed) s->clickwheel_tx_buf |= (1 << 0x14);

                            s->clickwheel_skip_cycle = 1;
                            s->clickwheel_clk = 0;
                            CLICKWHEEL_DEBUG("clickwheel read button presses, buffered: 0x%08x\n", s->clickwheel_tx_buf);
                            break;
                        default:
                            break;
                    }
                }
            }
            CLICKWHEEL_DEBUG("clickwheel out: 0x%08x\n", out);
            return out;
            break;
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

static void S5L8702_gpio_init(Object *obj) {
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    IPodNano3GGPIOState *s = IPOD_NANO3G_GPIO(dev);

    memory_region_init_io(&s->iomem, obj, &gpio_ops, s, "gpio", 0x10000);
}

static void S5L8702_gpio_class_init(ObjectClass *klass, void *data) {
}

static const TypeInfo ipod_nano3g_gpio_info = {
    .name          = TYPE_IPOD_NANO3G_GPIO,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GGPIOState),
    .instance_init = S5L8702_gpio_init,
    .class_init    = S5L8702_gpio_class_init,
};

static void ipod_nano3g_machine_types(void) {
    type_register_static(&ipod_nano3g_gpio_info);
}

type_init(ipod_nano3g_machine_types)
