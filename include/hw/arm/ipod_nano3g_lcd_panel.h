#ifndef IPOD_NANO3G_LCD_PANEL_H
#define IPOD_NANO3G_LCD_PANEL_H

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/ssi/ssi.h"

#define TYPE_IPOD_NANO3G_LCD_PANEL                "ipodnano3g.lcdpanel"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GLCDPanelState, IPOD_NANO3G_LCD_PANEL)

typedef struct IPodNano3GLCDPanelState {
    SSIPeripheral ssidev;
    uint32_t cur_cmd;
} IPodNano3GLCDPanelState;

#endif