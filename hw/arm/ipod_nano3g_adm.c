#include "hw/arm/ipod_nano3g_adm.h"
#include "hw/qdev-properties.h"
#include "hw/arm/ipod_nano3g_nand.h"
#include "qapi/error.h"

static void set_bank(ITNandState *s, uint8_t activate_bank) {
    for(int bank = 0; bank < 8; bank++) {
        // clear bit, toggle if it is active
        s->fmctrl0 &= ~(1 << (bank + 1));
        if(bank == activate_bank) {
            s->fmctrl0 ^= 1 << (bank + 1);
        }
    }
}

static uint64_t ipod_nano3g_adm_read(void *opaque, hwaddr offset, unsigned size)
{
    //IPodNano3GADMState *s = (IPodNano3GADMState *)opaque;

    //fprintf(stderr, "S5L8702_adm_read(): offset = 0x%08x\n", offset);

    switch (offset) {
        case ADM_CTRL: // this seems to be the control register
            return 0x2; // this seems to indicate that the device is ready
        case ADM_CTRL2:
            return 0x10; // indicates that a upload event has been finished
        default:
            break;
    }

    return 0;
}

static void ipod_nano3g_adm_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    IPodNano3GADMState *s = (IPodNano3GADMState *)opaque;

    //fprintf(stderr, "S5L8702_adm_write: offset = 0x%08x, val = 0x%08x\n", offset, value);

    switch(offset) {
        case ADM_CTRL:
            if(value == 0x3) {
                // some kind of start-up command?
                // write some bits to data2_sec_addr to indicate that the device is started
                uint32_t *data = (uint32_t *) malloc(sizeof(uint32_t));
                data[0] = 0x50;
                address_space_rw(&s->downstream_as, s->data2_sec_addr, MEMTXATTRS_UNSPECIFIED, (uint8_t *)data, 4, 1);

                // dunno, write some bytes to data4_sec_addr to indicate that the NAND banks are ready
                data = (uint32_t *) malloc(sizeof(uint32_t) * 8);
                for(int i = 0; i < 8; i++) {
                    data[i] = NAND_CHIP_ID;
                }
                
                address_space_rw(&s->downstream_as, s->data3_sec_addr, MEMTXATTRS_UNSPECIFIED, (uint8_t *)data, 8 * sizeof(uint32_t), 1);
            }
            break;
        case ADM_CTRL2:
            if(value == 0x2) {
                // read the command and initialize the right device
                uint32_t page;
                uint16_t num_pages;
                uint8_t bank;
                uint32_t *buf = malloc(80);
                address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x24, MEMTXATTRS_UNSPECIFIED, buf, 80);
                int cmd = *buf;
                // printf("Setting command: 0x%08x\n", cmd);
                // for(int i = 0; i < 20; i++) {
                //     printf("0x%08x ", buf[i]);
                // }
                // printf("\n");
                switch(cmd) {
                    case 0x200:
                        // read multiple pages simultaneously from the same bank
                        s->nand_state->reading_multiple_pages = true;
                        buf = malloc(2);
                        address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x28, MEMTXATTRS_UNSPECIFIED, buf, 2);
                        num_pages = *(uint16_t*)buf;
                        num_pages = ( (((num_pages) >> 8) & 0x00FF) | (((num_pages) << 8) & 0xFF00) ); // swap endianness
                        //printf("Reading %d pages at once, ", num_pages);

                        buf = malloc(4);
                        address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x244, MEMTXATTRS_UNSPECIFIED, buf, 4);
                        page = *buf;
                        page = ((page>>24)&0xff) | // move byte 3 to byte 0
                               ((page<<8)&0xff0000) | // move byte 1 to byte 2
                               ((page>>8)&0xff00) | // move byte 2 to byte 1
                               ((page<<24)&0xff000000); // byte 0 to byte 3
                        //printf("starting with page %d\n", page);

                        uint16_t ops = num_pages / 8;
                        for(int op = 0; op < ops; op++) {
                            for(int i = 0; i < 8; i++) {
                                s->nand_state->pages_to_read[op * 8 + i] = page;
                                s->nand_state->banks_to_read[op * 8 + i] = i;
                            }
                            page++;
                        }

                        s->nand_state->fmdnum = (num_pages * 0x800);
                        s->nand_state->cur_bank_reading = -1;

                        for(int i = 0; i < num_pages; i++) {
                            uint8_t *sbuf = malloc(0xC);
                            sbuf[10] = 0xFF;
                            address_space_rw(&s->downstream_as, s->data3_sec_addr + i * 0xC, MEMTXATTRS_UNSPECIFIED, sbuf, 0xC, 1);
                        }
                        
                        break;
                    case 0x300:
                        // seems to be the NAND read command, read the page(s) + bank and instruct the flash device
                        s->nand_state->reading_multiple_pages = false;
                        buf = malloc(2);
                        address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x28, MEMTXATTRS_UNSPECIFIED, buf, 2);
                        num_pages = *(uint16_t*)buf;
                        num_pages = ( (((num_pages) >> 8) & 0x00FF) | (((num_pages) << 8) & 0xFF00) ); // swap endianness
                        if(num_pages == 1) {
                            // TODO this can probably be refactored to re-use the logic to read multiple pages!
                            address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x44, MEMTXATTRS_UNSPECIFIED, buf, 1);
                            bank = *(uint8_t*)buf;

                            buf = malloc(4);
                            address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x244, MEMTXATTRS_UNSPECIFIED, buf, 4);
                            page = *buf;
                            page = ((page>>24)&0xff) | // move byte 3 to byte 0
                                   ((page<<8)&0xff0000) | // move byte 1 to byte 2
                                   ((page>>8)&0xff00) | // move byte 2 to byte 1
                                   ((page<<24)&0xff000000); // byte 0 to byte 3
                            //printf("Reading single page: %d (bank: %d)\n", page, bank);

                            // set the bank, page, and operation.
                            set_bank(s->nand_state, bank);
                            memory_region_dispatch_write(&s->nand_state->iomem, NAND_FMDNUM, 0x800 - 1, MO_32, MEMTXATTRS_UNSPECIFIED);
                            memory_region_dispatch_write(&s->nand_state->iomem, NAND_FMADDR0, page << 16, MO_32, MEMTXATTRS_UNSPECIFIED);
                            memory_region_dispatch_write(&s->nand_state->iomem, NAND_FMADDR1, (page >> 16) & 0xFF, MO_32, MEMTXATTRS_UNSPECIFIED);
                            memory_region_dispatch_write(&s->nand_state->iomem, NAND_CMD, NAND_CMD_READ, MO_32, MEMTXATTRS_UNSPECIFIED);

                            // write the spare of the page to the 3rd data section
                            nand_set_buffered_page(s->nand_state, page);
                            address_space_rw(&s->downstream_as, s->data3_sec_addr, MEMTXATTRS_UNSPECIFIED, (uint8_t *)s->nand_state->page_spare_buffer, NAND_BYTES_PER_SPARE, 1);
                        }
                        else if(num_pages > 1) {
                            // read scattered pages
                            // printf("Reading %d scattered pages\n", num_pages);
                            s->nand_state->reading_multiple_pages = true;
                            for(int i = 0; i < num_pages; i++) {
                                buf = malloc(4);
                                address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x244 + 4 * i, MEMTXATTRS_UNSPECIFIED, buf, 4);
                                page = *buf;
                                page = ((page>>24)&0xff) | // move byte 3 to byte 0
                                       ((page<<8)&0xff0000) | // move byte 1 to byte 2
                                       ((page>>8)&0xff00) | // move byte 2 to byte 1
                                       ((page<<24)&0xff000000); // byte 0 to byte 3

                                buf = malloc(1);
                                address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x44 + i, MEMTXATTRS_UNSPECIFIED, buf, 1);
                                bank = *(uint8_t*)buf;
                                // printf("Page: %d, bank: %d\n", page, bank);

                                s->nand_state->pages_to_read[i] = page;
                                s->nand_state->banks_to_read[i] = bank;
                            }

                            s->nand_state->fmdnum = (num_pages * 0x800);
                            s->nand_state->cur_bank_reading = -1;

                            for(int i = 0; i < num_pages; i++) {
                                uint8_t *sbuf = malloc(0xC);
                                sbuf[10] = 0xFF;
                                address_space_rw(&s->downstream_as, s->data3_sec_addr + i * 0xC, MEMTXATTRS_UNSPECIFIED, sbuf, 0xC, 1);
                            }
                        }
                        break;
                    case 0x500:
                        // writing a page
                        buf = malloc(1);
                        address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x44, MEMTXATTRS_UNSPECIFIED, buf, 1);
                        bank = *(uint8_t*)buf;

                        buf = malloc(4);
                        address_space_read(&s->downstream_as, s->data2_sec_addr + 0x1104 + 0x244, MEMTXATTRS_UNSPECIFIED, buf, 4);
                        page = *buf;
                        page = ((page>>24)&0xff) | // move byte 3 to byte 0
                               ((page<<8)&0xff0000) | // move byte 1 to byte 2
                               ((page>>8)&0xff00) | // move byte 2 to byte 1
                               ((page<<24)&0xff000000); // byte 0 to byte 3

                        // set the bank, page, and operation.
                        //printf("Activating bank for writing: %d, page: %d\n", bank, page);
                        set_bank(s->nand_state, bank);
                        nand_set_buffered_page(s->nand_state, page);
                        s->nand_state->fmdnum = NAND_BYTES_PER_PAGE;
                        s->nand_state->is_writing = true;
                        break;
                    default:
                        printf("Unrecognized ADM command: %d\n", cmd);
                        break;
                }
                qemu_irq_raise(s->irq);
            }
            if((value & 0x2) == 0) {
                qemu_irq_lower(s->irq);
            }
            break;
        case ADM_CODE_SEC_ADDR:
            s->code_sec_addr = value;
            break;
        case ADM_DATA1_SEC_ADDR:
            s->data1_sec_addr = value;
            break;
        case ADM_DATA2_SEC_ADDR:
            s->data2_sec_addr = value;
            break;
        case ADM_DATA3_SEC_ADDR:
            s->data3_sec_addr = value;
            break;
        default:
            break;
    }
}

static const MemoryRegionOps ipod_nano3g_adm_ops = {
    .read = ipod_nano3g_adm_read,
    .write = ipod_nano3g_adm_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void ipod_nano3g_adm_realize(DeviceState *dev, Error **errp)
{
    IPodNano3GADMState *s = IPOD_NANO3G_ADM(dev);

    if (!s->downstream) {
        error_setg(errp, "ADM 'downstream' link not set");
        return;
    }

    address_space_init(&s->downstream_as, s->downstream, "adm-downstream");
}

static Property adm_properties[] = {
    DEFINE_PROP_LINK("downstream", IPodNano3GADMState, downstream,
                     TYPE_MEMORY_REGION, MemoryRegion *),
    DEFINE_PROP_END_OF_LIST(),
};

static void ipod_nano3g_adm_init(Object *obj)
{
    IPodNano3GADMState *s = IPOD_NANO3G_ADM(obj);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &ipod_nano3g_adm_ops, s, TYPE_IPOD_NANO3G_ADM, 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void ipod_nano3g_adm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    dc->realize = ipod_nano3g_adm_realize;
    device_class_set_props(dc, adm_properties);
}

static const TypeInfo ipod_nano3g_adm_type_info = {
    .name = TYPE_IPOD_NANO3G_ADM,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(IPodNano3GADMState),
    .instance_init = ipod_nano3g_adm_init,
    .class_init = ipod_nano3g_adm_class_init,
};

static void ipod_nano3g_adm_register_types(void)
{
    type_register_static(&ipod_nano3g_adm_type_info);
}

type_init(ipod_nano3g_adm_register_types)
