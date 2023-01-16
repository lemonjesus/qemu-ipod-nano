#ifndef IPOD_TOUCH_LCD_H
#define IPOD_TOUCH_LCD_H

#include <math.h>
#include "qemu/osdep.h"
#include "qemu/fifo8.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "hw/arm/ipod_touch_multitouch.h"

#define TYPE_IPOD_TOUCH_LCD                "ipodtouch.lcd"
OBJECT_DECLARE_SIMPLE_TYPE(IPodTouchLCDState, IPOD_TOUCH_LCD)

#define LCD_REFRESH_RATE_FREQUENCY 10

#define LCD_CONFIG (0x000)
#define LCD_WCMD   (0x004)
#define LCD_RCMD   (0x00c)
#define LCD_RDATA  (0x010)
#define LCD_DBUFF  (0x014)
#define LCD_INTCON (0x018)
#define LCD_STATUS (0x01c)
#define LCD_PHTIME (0x020)
#define LCD_WDATA  (0x040)

typedef struct IPodTouchLCDState
{
    SysBusDevice parent_obj;
    MemoryRegion *sysmem;
    MemoryRegion iomem;
    QemuConsole *con;
    AddressSpace *nsas;
    IPodTouchMultitouchState *mt;
    int invalidate;
    MemoryRegionSection fbsection;
    qemu_irq irq;

    Fifo8* dbuff_buf;

    uint32_t lcd_config;
    uint32_t lcd_wcmd;
    uint32_t lcd_rcmd;
    uint32_t lcd_rdata;
    uint32_t lcd_dbuff;
    uint32_t lcd_intcon;
    uint32_t lcd_status;
    uint32_t lcd_phtime;
    uint32_t lcd_wdata;

    uint64_t* lcd_regs; // internal registers in case we ever need to access them in the future
    
    uint16_t* framebuffer;
    uint64_t memcnt;

    uint32_t unknown1;
    uint32_t unknown2;

    uint32_t wnd_con;

    uint32_t vid_con0;
    uint32_t vid_con1;

    uint32_t vidt_con0;
    uint32_t vidt_con1;
    uint32_t vidt_con2;
    uint32_t vidt_con3;

    uint32_t w1_hspan;
    uint32_t w1_framebuffer_base;
    uint32_t w1_display_resolution_info;
    uint32_t w1_display_depth_info;
    uint32_t w1_qlen;

    uint32_t w2_hspan;
    uint32_t w2_framebuffer_base;
    uint32_t w2_display_resolution_info;
    uint32_t w2_display_depth_info;
    uint32_t w2_qlen;

    QEMUTimer *refresh_timer;
} IPodTouchLCDState;

#endif