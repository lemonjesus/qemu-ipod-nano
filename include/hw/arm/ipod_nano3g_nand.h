#ifndef HW_ARM_IPOD_NANO3G_NAND_H
#define HW_ARM_IPOD_NANO3G_NAND_H

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/platform-bus.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "qemu/lockable.h"

#define NAND_NUM_BANKS 8
#define NAND_BYTES_PER_PAGE 2048
#define NAND_BYTES_PER_SPARE 64

#define NAND_CHIP_ID 0xA514D3AD

#define NAND_FMCTRL0  0x0
#define NAND_FMCTRL1  0x4
#define NAND_CMD      0x8
#define NAND_FMADDR0  0xC
#define NAND_FMADDR1  0x10
#define NAND_FMANUM   0x2C
#define NAND_FMDNUM   0x30
#define NAND_FMCSTAT  0x48
#define NAND_FMFIFO   0x80
#define NAND_RSCTRL   0x100

#define NAND_CMD_ID  0x90
#define NAND_CMD_READ 0x30
#define NAND_CMD_READSTATUS 0x70

#define FILESYSTEM_START_VPN 206851
#define FILESYSTEM_NUM_PAGES 132854

#define TYPE_ITNAND "itnand"
OBJECT_DECLARE_SIMPLE_TYPE(ITNandState, ITNAND)

typedef struct ITNandState {
    SysBusDevice busdev;
    MemoryRegion iomem;
    uint32_t fmctrl0;
    uint32_t fmctrl1;
    uint32_t fmaddr0;
    uint32_t fmaddr1;
    uint32_t fmanum;
    uint32_t fmdnum;
	uint32_t rsctrl;
	uint32_t cmd;
	uint8_t reading_spare;
    qemu_irq irq;

    uint8_t *page_buffer;
    uint8_t *page_spare_buffer;
    uint32_t buffered_bank;
    uint32_t buffered_page;
    bool reading_multiple_pages;
    uint32_t cur_bank_reading;
    uint32_t banks_to_read[512]; // used when in multiple page read mode
    uint32_t pages_to_read[512]; // used when in multiple page read mode
    bool is_writing;
    QemuMutex lock;
    char *nand_path;
} ITNandState;

void nand_set_buffered_page(ITNandState *s, uint32_t page);

#endif