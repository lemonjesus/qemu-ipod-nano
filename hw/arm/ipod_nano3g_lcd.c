#include "hw/arm/ipod_nano3g_lcd.h"
#include "ui/pixel_ops.h"
#include "ui/console.h"
#include "hw/display/framebuffer.h"

#define LCD_CONFIG (0x000)
#define LCD_WCMD   (0x004)
#define LCD_RCMD   (0x00c)
#define LCD_RDATA  (0x010)
#define LCD_DBUFF  (0x014)
#define LCD_INTCON (0x018)
#define LCD_STATUS (0x01c)
#define LCD_PHTIME (0x020)
#define LCD_WDATA  (0x040)

static uint64_t S5L8702_lcd_read(void *opaque, hwaddr addr, unsigned size) {
    IPodNano3GLCDState *s = (IPodNano3GLCDState *)opaque;
    uint64_t r = 0;
    switch(addr)
    {
        case LCD_CONFIG:
            r = s->lcd_config;
            break;
        case LCD_WCMD:
            r = s->lcd_wcmd;
            break;
        case LCD_RCMD:
            r = s->lcd_rcmd;
            break;
        case LCD_RDATA:
            r = s->lcd_rdata;
            break;
        case LCD_DBUFF:
            r = s->lcd_dbuff;
            break;
        case LCD_INTCON:
            r = s->lcd_intcon;
            break;
        case LCD_STATUS:
            r = 0xFFFFFFEF;
            break;
        case LCD_PHTIME:
            r = s->lcd_phtime;
            break;
        case LCD_WDATA:
            r = s->lcd_wdata;
            break;
        default:
            hw_error("%s: read invalid location 0x%08x.\n", __func__, addr);
            break;
    }
    // fprintf(stderr, "LCD read  0x%08x <- 0x%08x\n", r, addr);
    return r;
}

static void S5L8702_lcd_write(void *opaque, hwaddr addr, uint64_t val, unsigned size)
{
    IPodNano3GLCDState *s = (IPodNano3GLCDState *)opaque;
    // fprintf(stderr, "LCD write 0x%08x -> 0x%08x\n", val, addr);

    switch(addr) {
        case LCD_CONFIG:
            s->lcd_config = val;
            break;
        case LCD_WCMD:
            s->lcd_wcmd = val;
            // if(val < 0x2A || val > 0x2C) printf("LCD Got Command 0x%08x\n", s->lcd_wcmd);
            switch(s->lcd_wcmd) {
                case 0x04:
                    fifo8_reset(s->dbuff_buf);
                    fifo8_push(s->dbuff_buf, 0x00);
                    fifo8_push(s->dbuff_buf, 0x38);
                    fifo8_push(s->dbuff_buf, 0xB3);
                    fifo8_push(s->dbuff_buf, 0x71);
                    break;
                case 0x2c:
                    s->address_latches = 0;
                    s->memcnt = 0;
                    break;
                case 0x28:
                    printf("DISPLAY OFF\n");
                    break;
                case 0x29:
                    printf("DISPLAY ON\n");
                    break;
            }
            break;
        case LCD_RCMD:
            s->lcd_rcmd = val;
            break;
        case LCD_RDATA:
            s->lcd_rdata = val;
            if(val == 0) {
                if(fifo8_is_empty(s->dbuff_buf)) s->lcd_dbuff = 0;
                else s->lcd_dbuff = fifo8_pop(s->dbuff_buf) << 1;
            }
            break;
        case LCD_DBUFF:
            s->lcd_dbuff = val;
            break;
        case LCD_INTCON:
            s->lcd_intcon = val;
            break;
        case LCD_STATUS:
            s->lcd_status = val;
            break;
        case LCD_PHTIME:
            s->lcd_phtime = val;
            break;
        case LCD_WDATA:
            s->lcd_wdata = val;
            switch(s->lcd_wcmd) {
            case 0x2A:
                if(s->address_latches < 2) s->sc = (s->sc << 8) | val;
                else s->ec = (s->ec << 8) | val;
                s->address_latches++;
                if(s->address_latches == 4) {
                    s->address_latches = 0;
                    printf("LCD GOT 0x2A: sc=%04x ec=%04x\n", s->sc, s->ec);
                }
                break;
            case 0x2B:
                if(s->address_latches < 2) s->sp = (s->sp << 8) | val;
                else s->ep = (s->ep << 8) | val;
                s->address_latches++;
                if(s->address_latches == 4) {
                    s->address_latches = 0;
                    printf("LCD GOT 0x2B: sp=%04x ep=%04x\n", s->sp, s->ep);
                }

                break;
            case 0x2C:
                uint32_t address;
                // this simulates writing pixels as if we were a ILI9341. we start at the top left corner of the column and page defined by sc and sp
                // and write pixels until we reach the bottom right corner of the column and page defined by ec and ep. we keep track of the current
                // pixel we're on with s->memcnt and increment it every time we write a pixel. we use this to calculate the address of the pixel we're
                // writing to in the framebuffer. if we reach the end of the page (memcnt > ec - sc) we increment the page and reset the column.

                address = (s->sp * 320) + s->sc + s->memcnt;
                if(s->memcnt > s->ec - s->sc) {
                    s->sp++;
                    // s->sc = 0;
                    s->memcnt = 0;
                    address = (s->sp * 320) + s->sc + s->memcnt;
                }
                
                address_space_rw(s->nsas, 0xfe00000 + address * 2, MEMTXATTRS_UNSPECIFIED, &val, 2, 1);
                s->invalidate = true;
                // printf("FB writing %08x to %08x\n", val, s->memcnt);
                s->memcnt++;
                break;
            case 0x3A:
                printf("LCD GOT 0x3A: %04x\n", val);
                break;
            default:
                s->lcd_regs[s->lcd_wcmd] = s->lcd_regs[s->lcd_wcmd] << 8 | (val & 0xFF);
                // fprintf(stderr, "LCD Register 0x%02x = 0x%016llx\n", s->lcd_wcmd, s->lcd_regs[s->lcd_wcmd]);
                break;
            }
            break;
        default:
            hw_error("%s: write invalid location 0x%08x.\n", __func__, addr);
            break;
    }
}

static void lcd_invalidate(void *opaque)
{
    IPodNano3GLCDState *s = opaque;
    s->invalidate = 1;
}

static void draw_line32_32(void *opaque, uint8_t *d, const uint8_t *s, int width, int deststep)
{
    uint8_t r, g, b;

    do {
        uint16_t v = lduw_le_p((void *) s);
        //printf("V: %d\n", *s);
        // convert 5-6-5 to 8-8-8
        // uint16_t v = ((uint16_t *) s)[0];
        r = (uint8_t)(((v & 0xF800) >> 11) << 3);
        g = (uint8_t)(((v & 0x7E0) >> 5) << 2);
        b = (uint8_t)(((v & 0x1F)) << 3);
        // if(r > 0 && r < 0xFF) printf("R: %d, G: %d, B: %d\n", r, g, b);
        ((uint32_t *) d)[0] = rgb_to_pixel32(r, g, b);
        s += 2;
        d += 4;
    } while (-- width != 0);
}

static void lcd_refresh(void *opaque)
{
    //fprintf(stderr, "%s: refreshing LCD screen\n", __func__);

    IPodNano3GLCDState *lcd = (IPodNano3GLCDState *) opaque;
    DisplaySurface *surface = qemu_console_surface(lcd->con);
    drawfn draw_line;
    int src_width, dest_width;
    int height, first, last;
    int width, linesize;

    if (!lcd || !lcd->con || !surface_bits_per_pixel(surface))
        return;

    dest_width = 4;
    draw_line = draw_line32_32;

    /* Resolution */
    first = last = 0;
    width = 320;
    height = 240;
    lcd->invalidate = 1;

    src_width =  2 * width;
    linesize = surface_stride(surface);

    if(lcd->invalidate) {
        framebuffer_update_memory_section(&lcd->fbsection, lcd->sysmem, 0xfe00000, height, src_width);
    }

    framebuffer_update_display(surface, &lcd->fbsection,
                               width, height,
                               src_width,       /* Length of source line, in bytes.  */
                               linesize,        /* Bytes between adjacent horizontal output pixels.  */
                               dest_width,      /* Bytes between adjacent vertical output pixels.  */
                               lcd->invalidate,
                               draw_line, NULL,
                               &first, &last);
    if (first >= 0) {
        dpy_gfx_update(lcd->con, 0, first, width, last - first + 1);
    }
    lcd->invalidate = 0;
}

static const MemoryRegionOps lcd_ops = {
    .read = S5L8702_lcd_read,
    .write = S5L8702_lcd_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const GraphicHwOps S5L8702_gfx_ops = {
    .invalidate  = lcd_invalidate,
    .gfx_update  = lcd_refresh,
};

static void ipod_nano3g_lcd_mouse_event(void *opaque, int x, int y, int z, int buttons_state)
{
    // TODO: maybe use this in the future for the non-button part of the clickwheel?
    // printf("CLICKY %d %d %d %d\n", x, y, z, buttons_state);
}

static void refresh_timer_tick(void *opaque)
{
    IPodNano3GLCDState *s = (IPodNano3GLCDState *)opaque;

    qemu_irq_raise(s->irq);

    timer_mod(s->refresh_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + NANOSECONDS_PER_SECOND / LCD_REFRESH_RATE_FREQUENCY);
}

static void S5L8702_lcd_realize(DeviceState *dev, Error **errp)
{
    IPodNano3GLCDState *s = IPOD_NANO3G_LCD(dev);
    s->con = graphic_console_init(dev, 0, &S5L8702_gfx_ops, s);
    qemu_console_resize(s->con, 320, 240);

    // add mouse handler
    // qemu_add_mouse_event_handler(ipod_nano3g_lcd_mouse_event, s, 1, "iPod Touch Touchscreen");

    // initialize the refresh timer
    s->refresh_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, refresh_timer_tick, s);
    timer_mod(s->refresh_timer, qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + NANOSECONDS_PER_SECOND / LCD_REFRESH_RATE_FREQUENCY);

    // initialize the dbuff buffer
    s->dbuff_buf = g_malloc0(sizeof(Fifo8));
    fifo8_create(s->dbuff_buf, 0x4);

    // initialize the lcd's internal registers (they're all just uint64_t's)
    s->lcd_regs = g_malloc0(sizeof(uint64_t) * 0xFF);

    // initialize the lcd's internal framebuffer
    s->framebuffer = g_malloc0(sizeof(uint16_t) * 320 * 240);
}

static void S5L8702_lcd_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    IPodNano3GLCDState *s = IPOD_NANO3G_LCD(dev);

    memory_region_init_io(&s->iomem, obj, &lcd_ops, s, "lcd", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void S5L8702_lcd_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = S5L8702_lcd_realize;
}

static const TypeInfo ipod_nano3g_lcd_info = {
    .name          = TYPE_IPOD_NANO3G_LCD,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GLCDState),
    .instance_init = S5L8702_lcd_init,
    .class_init    = S5L8702_lcd_class_init,
};

static void ipod_nano3g_machine_types(void)
{
    type_register_static(&ipod_nano3g_lcd_info);
}

type_init(ipod_nano3g_machine_types)