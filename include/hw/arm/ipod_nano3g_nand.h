#ifndef HW_ARM_IPOD_NANO3G_NAND_H
#define HW_ARM_IPOD_NANO3G_NAND_H

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "hw/platform-bus.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "qemu/lockable.h"
#include "file-cow.h"

#define NAND_NUM_BANKS 8
#define NAND_BYTES_PER_PAGE 2048
#define NAND_BYTES_PER_SPARE 64

#define NAND_CHIP_ID 0xA5D5D589
#define NAND_NUM_BANKS_INSTALLED 4

#define NAND_FMCTRL0  0x0
#define NAND_FMCTRL1  0x4
#define NAND_CMD      0x8
#define NAND_FMADDR0  0xC
#define NAND_FMADDR1  0x10
#define NAND_FMANUM   0x2C
#define NAND_FMDNUM   0x30
#define NAND_DESTADDR 0x34
#define NAND_FMCSTAT  0x48
#define NAND_FMFIFO   0x60
#define NAND_RSCTRL   0x100

#define FMI_PROGRAM 0xc04
#define FMI_INT 0xc0c
#define FMI_START 0xc00
#define FMI_DMEM 0xd00

#define NAND_CMD_ID  0x90
#define NAND_CMD_READ 0x30
#define NAND_CMD_READSTATUS 0x70

#define FILESYSTEM_START_VPN 206851
#define FILESYSTEM_NUM_PAGES 132854

#define TYPE_IPOD_NANO3G_NAND "ipodnano3g.nand"
OBJECT_DECLARE_SIMPLE_TYPE(NandState, IPOD_NANO3G_NAND)

#define FMIVSS_DMEM_SIZE 32
typedef struct {
    uint32_t regs[8];
    // Address _inside_ the main device's DMA memory!
    uint32_t pc;
    uint32_t start_pc;

    uint32_t dmem[FMIVSS_DMEM_SIZE];

    AddressSpace *iomem;
} fmiss_vm;

typedef struct NandState {
    SysBusDevice busdev;
    MemoryRegion iomem;
    uint32_t fmctrl0;
    uint32_t fmctrl1;
    uint32_t fmaddr0;
    uint32_t fmaddr1;
    uint32_t fmanum;
    uint32_t fmdnum;
    uint32_t destaddr;
	uint32_t rsctrl;
	uint32_t cmd;

    uint32_t fmi_c00;
    uint32_t fmi_program;
    uint32_t fmi_int;
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
    cow_file** nand_banks;
    cow_file** nand_spares;

    uint32_t spare_buffer[3];

    MemoryRegion *sysmem;
    MemoryRegion *downstream;
    AddressSpace *downstream_as;

    fmiss_vm fmiss_vm;
} NandState;

void nand_set_buffered_page(NandState *s, uint32_t page);

#endif