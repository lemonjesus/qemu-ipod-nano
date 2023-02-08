#include "hw/arm/ipod_nano3g_lcd_panel.h"

static uint32_t ipod_nano3g_lcd_panel_transfer(SSIPeripheral *dev, uint32_t value)
{
    IPodNano3GLCDPanelState *s = IPOD_NANO3G_LCD_PANEL(dev);

    if(!s->cur_cmd && (value == 0x95 || value == 0xDA || value == 0xDB || value == 0xDC)) {
        // this is a command -> set it
        s->cur_cmd = value;
        return 0x0;
    }

    if(s->cur_cmd) {
        uint32_t res = 0;
        switch(s->cur_cmd) {
        case 0x95:
            res = 0x1;
            break;
        case 0xDA:
            res = 0x71;
            break;
        case 0xDB:
            res = 0xC2;
            break;
        case 0xDC:
            res = 0x0;
            break;
        default:
            break;
        }

        s->cur_cmd = 0;
        return res;
    }
    
    return 0x0;
}

static void ipod_nano3g_lcd_panel_realize(SSIPeripheral *d, Error **errp)
{

}

static void ipod_nano3g_lcd_panel_class_init(ObjectClass *klass, void *data)
{
    SSIPeripheralClass *k = SSI_PERIPHERAL_CLASS(klass);
    k->realize = ipod_nano3g_lcd_panel_realize;
    k->transfer = ipod_nano3g_lcd_panel_transfer;
}

static const TypeInfo ipod_nano3g_lcd_panel_type_info = {
    .name = TYPE_IPOD_NANO3G_LCD_PANEL,
    .parent = TYPE_SSI_PERIPHERAL,
    .instance_size = sizeof(IPodNano3GLCDPanelState),
    .class_init = ipod_nano3g_lcd_panel_class_init,
};

static void ipod_nano3g_lcd_panel_register_types(void)
{
    type_register_static(&ipod_nano3g_lcd_panel_type_info);
}

type_init(ipod_nano3g_lcd_panel_register_types)
