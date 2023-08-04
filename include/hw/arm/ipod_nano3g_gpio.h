#ifndef IPOD_NANO3G_GPIO_H
#define IPOD_NANO3G_GPIO_H

#include <math.h>
#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"

#define TYPE_IPOD_NANO3G_GPIO                "ipodnano3g.gpio"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GGPIOState, IPOD_NANO3G_GPIO)

#define GPIO_BUTTON_POWER 0x1605
#define GPIO_BUTTON_HOME  0x1606

#define GPIO_BUTTON_POWER_IRQ 0x2D
#define GPIO_BUTTON_HOME_IRQ  0x2E

#define GPIO_N_GROUPS  16
#define GPIO_N_PINS 0x20

#define GPIO_PCON0     0x000
#define GPIO_PDAT0     0x004
#define GPIO_PCON1     0x020
#define GPIO_PDAT1     0x024
#define GPIO_PCON2     0x040
#define GPIO_PDAT2     0x044
#define GPIO_PCON3     0x060
#define GPIO_PDAT3     0x064
#define GPIO_PCON4     0x080
#define GPIO_PDAT4     0x084
#define GPIO_PCON5     0x0a0
#define GPIO_PDAT5     0x0a4
#define GPIO_PCON6     0x0c0
#define GPIO_PDAT6     0x0c4
#define GPIO_PCON7     0x0e0
#define GPIO_PDAT7     0x0e4
#define GPIO_PCON8     0x100
#define GPIO_PDAT8     0x104
#define GPIO_PCON9     0x120
#define GPIO_PDAT9     0x124
#define GPIO_PCONA     0x140
#define GPIO_PDATA     0x144
#define GPIO_PCONB     0x160
#define GPIO_PDATB     0x164
#define GPIO_PCONC     0x180
#define GPIO_PDATC     0x184
#define GPIO_PCOND     0x1a0
#define GPIO_PDATD     0x1a4
#define GPIO_PCONE     0x1c0
#define GPIO_PDATE     0x1c4
#define GPIO_PCONF     0x1e0
#define GPIO_PDATF     0x1e4
#define GPIO_CMD       0x200

typedef struct IPodNano3GGPIOState
{
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    uint32_t gpio_state;
    uint8_t gpio_pin_state[GPIO_N_GROUPS][GPIO_N_PINS];

    uint32_t clickwheel_rx_buf;
    uint32_t clickwheel_tx_buf;
    uint8_t clickwheel_clk;
    uint8_t clickwheel_bit_to_send;
    uint8_t clickwheel_skip_cycle;

    uint8_t clickwheel_select_pressed;
    uint8_t clickwheel_menu_pressed;
    uint8_t clickwheel_play_pressed;
    uint8_t clickwheel_prev_pressed;
    uint8_t clickwheel_next_pressed;
} IPodNano3GGPIOState;

#endif