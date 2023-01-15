/*
 * S5L8900 SPI Emulation
 *
 * by cmw
 */

#include "hw/arm/ipod_touch_spi.h"

static int apple_spi_word_size(S5L8900SPIState *s)
{
    switch (R_CFG_WORD_SIZE(REG(s, R_CFG))) {
    case R_CFG_WORD_SIZE_8B:
        return 1;
    case R_CFG_WORD_SIZE_16B:
        return 2;
    case R_CFG_WORD_SIZE_32B:
        return 4;
    default:
        break;
    }
    g_assert_not_reached();
}

static void apple_spi_update_xfer_tx(S5L8900SPIState *s)
{
    if (fifo8_is_empty(&s->tx_fifo)) {
        REG(s, R_STATUS) |= R_STATUS_TXEMPTY;
    }
}

static void apple_spi_update_xfer_rx(S5L8900SPIState *s)
{
    if (!fifo8_is_empty(&s->rx_fifo)) {
        REG(s, R_STATUS) |= R_STATUS_RXREADY;
    }
}

static void apple_spi_update_irq(S5L8900SPIState *s)
{
    uint32_t irq = 0;
    uint32_t mask = 0;

    if (REG(s, R_CFG) & R_CFG_IE_RXREADY) {
        mask |= R_STATUS_RXREADY;
    }
    if (REG(s, R_CFG) & R_CFG_IE_TXEMPTY) {
        mask |= R_STATUS_TXEMPTY;
    }
    if (REG(s, R_CFG) & R_CFG_IE_COMPLETE) {
        mask |= R_STATUS_COMPLETE;
    }

    if (REG(s, R_STATUS) & mask) {
        irq = 1;
    }
    if (irq != s->last_irq) {
        s->last_irq = irq;
        qemu_set_irq(s->irq, irq);
    }
}

static void apple_spi_update_cs(S5L8900SPIState *s)
{
    BusState *b = BUS(s->spi);
    BusChild *kid = QTAILQ_FIRST(&b->children);
    if (kid) {
        // TODO GPIO not properly setup yet
        //qemu_set_irq(qdev_get_gpio_in_named(kid->child, SSI_GPIO_CS, 0), (REG(s, R_PIN) & R_PIN_CS) != 0);
    }
}

static void apple_spi_cs_set(void *opaque, int pin, int level)
{
    S5L8900SPIState *s = S5L8900SPI(opaque);
    if (level) {
        REG(s, R_PIN) |= R_PIN_CS;
    } else {
        REG(s, R_PIN) &= ~R_PIN_CS;
    }
    apple_spi_update_cs(s);
}

static void apple_spi_run(S5L8900SPIState *s)
{
    uint32_t tx;
    uint32_t rx;

    // fprintf(stderr, "apple_spi_run\n");

    if (!(REG(s, R_CTRL) & R_CTRL_RUN)) {
        // fprintf(stderr, "apple_spi_run: not running: R_CTRL\n");
        return;
    }

    apple_spi_update_xfer_tx(s);

    while (!fifo8_is_empty(&s->tx_fifo)) {
        tx = (uint32_t)fifo8_pop(&s->tx_fifo);
        rx = ssi_transfer(s->spi, tx);
        // REG(s, R_TXCNT)--;
        apple_spi_update_xfer_tx(s);
        if (REG(s, R_RXCNT) > 0) {
            if (fifo8_is_full(&s->rx_fifo)) {
                qemu_log_mask(LOG_GUEST_ERROR, "%s: rx overflow\n", __func__);
                REG(s, R_STATUS) |= R_STATUS_RXOVERFLOW;
            } else {
                fifo8_push(&s->rx_fifo, (uint8_t)rx);
                REG(s, R_RXCNT)--;
                apple_spi_update_xfer_rx(s);
            }
        }
    }

    // fetch the remaining bytes by sending sentinel bytes.
    while (!fifo8_is_full(&s->rx_fifo) && (REG(s, R_RXCNT) > 0) && (REG(s, R_CFG) & R_CFG_AGD)) {
        rx = ssi_transfer(s->spi, 0xff);
        if (fifo8_is_full(&s->rx_fifo)) {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: rx overflow\n", __func__);
            REG(s, R_STATUS) |= R_STATUS_RXOVERFLOW;
            break;
        } else {
            fifo8_push(&s->rx_fifo, (uint8_t)rx);
            REG(s, R_RXCNT)--;
            apple_spi_update_xfer_rx(s);
        }
    }
    if (REG(s, R_RXCNT) == 0 && REG(s, R_TXCNT) == 0) {
        REG(s, R_STATUS) |= R_STATUS_COMPLETE;
    }
}

static uint64_t s5l8900_spi_read(void *opaque, hwaddr addr, unsigned size)
{
    S5L8900SPIState *s = S5L8900SPI(opaque);

    uint32_t r;
    bool run = false;

    r = s->regs[addr >> 2];
    switch (addr) {
        case R_CTRL: {
            if(REG(s, R_CTRL) == 0) {
                r = 0x00000002;
            }
            break;
        }
        case R_RXDATA: {
            const uint8_t *buf = NULL;
            int word_size = apple_spi_word_size(s);
            uint32_t num = 0;
            if (fifo8_is_empty(&s->rx_fifo)) {
                qemu_log_mask(LOG_GUEST_ERROR, "%s: rx underflow\n", __func__);
                r = 0;
                break;
            }
            buf = fifo8_pop_buf(&s->rx_fifo, word_size, &num);
            memcpy(&r, buf, num);

            if (fifo8_is_empty(&s->rx_fifo)) {
                run = true;
            }
            break;
        }
        case R_STATUS: {
            int val = 0;
            val |= (fifo8_num_used(&s->tx_fifo) << R_STATUS_TXFIFO_SHIFT);
            val |= (fifo8_num_used(&s->rx_fifo) << R_STATUS_RXFIFO_SHIFT);
            r |= val;
            // fprintf(stderr, "val is %d, r is %d, R_CTRL is %d, R_STATUS is %d, R_RXCNT is %d, R_TXCNT is %d, R_CFG is %d, R_PIN is %d, R_TXDATA is %d, R_RXDATA is %d\n", val, r, REG(s, R_CTRL), REG(s, R_STATUS), REG(s, R_RXCNT), REG(s, R_TXCNT), REG(s, R_CFG), REG(s, R_PIN), REG(s, R_TXDATA), REG(s, R_RXDATA));
            r = 0x3e00;
            if(REG(s, R_CTRL) & 0xC) {
                 if(REG(s, R_TXDATA) == 0xFF && val > 0) r = 0xFF0; // the nor was probably just reset
                 else if(REG(s, R_TXDATA) == 0xFF && r != REG(s, R_STATUS)) r = 0x0;
                 else if(REG(s, R_TXDATA) == 0xFF && val == 0 && REG(s, R_RXCNT) == 0) r = 0xF0;
                 else r = 0;
            }
            break;
        }
        default:
            break;
    }

    // fprintf(stderr, "%s (base %d): read from location 0x%08x -> 0x%08X\n", __func__, s->base, addr, r);

    if (run) {
        apple_spi_run(s);
    }
    apple_spi_update_irq(s);
    return r;
}

static void s5l8900_spi_write(void *opaque, hwaddr addr, uint64_t data, unsigned size)
{
    S5L8900SPIState *s = S5L8900SPI(opaque);
    // fprintf(stderr, "%s (base %d): writing 0x%08x to 0x%08x\n", __func__, s->base, data, addr);

    uint32_t r = data;
    uint32_t *mmio = &REG(s, addr);
    uint32_t old = *mmio;
    bool cs_flg = false;
    bool run = false;

    switch (addr) {
    case R_CTRL:
        if (r & R_CTRL_TX_RESET) {
            fifo8_reset(&s->tx_fifo);
        }
        if (r & R_CTRL_RX_RESET) {
            fifo8_reset(&s->rx_fifo);
        }
        if (r & R_CTRL_RUN && !fifo8_is_empty(&s->tx_fifo)) {
            run = true;
        }
        if(r & 0xC) {
            REG(s, R_STATUS) |= 0xF0;
        }
        if (r == 0) {
            REG(s, R_CTRL) = 2;
        }
        break;
    case R_STATUS:
        r = old & (~r);
        break;
    case R_PIN:
        cs_flg = true;
        break;
    case R_TXDATA ... R_TXDATA + 3: {
        int word_size = apple_spi_word_size(s);
        if ((fifo8_is_full(&s->tx_fifo)) || (fifo8_num_free(&s->tx_fifo) < word_size)) {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: tx overflow\n", __func__);
            r = 0;
            break;
        }
        fifo8_push_all(&s->tx_fifo, (uint8_t *)&r, word_size);
        if(REG(s, R_CTRL) & R_CTRL_RUN) run = true;
        break;
    case R_CFG:
        run = true;
        break;
    }
    default:
        break;
    }

    *mmio = r;
    if (cs_flg) {
        apple_spi_update_cs(s);
    }
    if (run) {
        apple_spi_run(s);
    }

    if(addr == R_STATUS) {
        apple_spi_update_xfer_tx(s);
        apple_spi_update_xfer_rx(s);
    }

    apple_spi_update_irq(s);
}

static const MemoryRegionOps spi_ops = {
    .read = s5l8900_spi_read,
    .write = s5l8900_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void s5l8900_spi_reset(DeviceState *d)
{
    S5L8900SPIState *s = (S5L8900SPIState *)d;
	memset(s->regs, 0, sizeof(s->regs));
    fifo8_reset(&s->tx_fifo);
    fifo8_reset(&s->rx_fifo);
}

static uint32_t base_addr = 0;

void set_spi_base(uint32_t base)
{
	base_addr = base;
}

static void s5l8900_spi_realize(DeviceState *dev, struct Error **errp)
{
    S5L8900SPIState *s = S5L8900SPI(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    char bus_name[32] = { 0 };
    snprintf(bus_name, sizeof(bus_name), "%s.bus", dev->id);
    s->spi = ssi_create_bus(dev, (const char *)bus_name);

    sysbus_init_irq(sbd, &s->irq);
    sysbus_init_irq(sbd, &s->cs_line);
    qdev_init_gpio_in_named(dev, apple_spi_cs_set, SSI_GPIO_CS, 1);
    char name[5];
    snprintf(name, 5, "spi%d", base_addr);
    memory_region_init_io(&s->iomem, OBJECT(s), &spi_ops, s, name, 0x100);
    sysbus_init_mmio(sbd, &s->iomem);
    s->base = base_addr;

    fifo8_create(&s->tx_fifo, R_FIFO_DEPTH);
    fifo8_create(&s->rx_fifo, R_FIFO_DEPTH);

    // create the peripheral
    switch(s->base) {
        case 0:
            dev = ssi_create_peripheral(s->spi, TYPE_IPOD_TOUCH_NOR_SPI);
            IPodTouchNORSPIState *nor = IPOD_TOUCH_NOR_SPI(dev);
            s->nor = nor;
            break;
        case 1:
            ssi_create_peripheral(s->spi, TYPE_IPOD_TOUCH_LCD_PANEL);
            break;
        case 2:
        {
            DeviceState *dev = ssi_create_peripheral(s->spi, TYPE_IPOD_TOUCH_MULTITOUCH);
            IPodTouchMultitouchState *mt = IPOD_TOUCH_MULTITOUCH(dev);
            s->mt = mt;
            break;
        }
    }
}

static void s5l8900_spi_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = s5l8900_spi_realize;
    dc->reset = s5l8900_spi_reset;
}

static const TypeInfo s5l8900_spi_info = {
    .name          = TYPE_S5L8900SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S5L8900SPIState),
    .class_init    = s5l8900_spi_class_init,
};

static void s5l8900_spi_register_types(void)
{
    type_register_static(&s5l8900_spi_info);
}

type_init(s5l8900_spi_register_types)