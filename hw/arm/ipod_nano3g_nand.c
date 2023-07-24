#include "hw/arm/ipod_nano3g_nand.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"

#define ENABLE_FMISS_DEBUG 1
#define FMISS_DEBUG if (ENABLE_FMISS_DEBUG) printf

static uint64_t nand_mem_read(void *opaque, hwaddr addr, unsigned size);
static void nand_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size);

static void fmiss_vm_reset(fmiss_vm *vm, uint32_t pc) {
    for (int i = 0; i < 8; i++) {
        vm->regs[i] = 0;
    }
    vm->start_pc = pc;
    vm->pc = pc;
}

static uint64_t fmiss_vm_fetch(fmiss_vm *vm) {
    uint64_t val = 0;
    address_space_read(vm->iomem, vm->pc, MEMTXATTRS_UNSPECIFIED, &val, 8);
    return val;
}

// Reference: https://github.com/lemonjesus/S5L8702-FMISS-Tools/blob/main/Documentation.md
static bool fmiss_vm_step(void *opaque, fmiss_vm *vm) {
    uint64_t ins = fmiss_vm_fetch(vm);

    uint32_t imm = ins >> 32;
    uint8_t opcode = ins >> 24;
    uint8_t dst = ins >> 16;
    uint16_t src = ins;

    uint8_t dst_reg = dst % 8;
    uint8_t src_reg = src % 8;
    
    FMISS_DEBUG("fmiss_vm: at %08x: %016lx (%02x %02x %04x %08x)\n", vm->pc - vm->start_pc, ins, opcode, dst, src, imm);
    switch (opcode) {
    case 0x0: // Terminate.
        FMISS_DEBUG("fmiss_vm: terminate\n");
        return false;
    case 0x1: // Write immediate to memory.
        FMISS_DEBUG("fmiss_vm: *0x%08x <- 0x%08x (immediate)\n", src + 0x38A00000, imm);
        nand_mem_write(opaque, src, imm, 4);
        break;
    case 0x2: // Write register to memory.
        FMISS_DEBUG("fmiss_vm: *0x%08x <- 0x%08x (r%d)\n", src + 0x38A00000, vm->regs[dst_reg], dst_reg);
        nand_mem_write(opaque, src, vm->regs[dst_reg], 4);
        break;
    case 0x3: // Dereference address in register
        address_space_read(vm->iomem, vm->regs[src_reg] ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, &(vm->regs[dst_reg]), 4);
        FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (*0x%08x from r%d)\n", vm->regs[dst_reg], dst_reg, vm->regs[src_reg] ^ 0x80000000, src_reg);
        break;
    case 0x4: // Read from memory into a register.
        uint32_t val = nand_mem_read(opaque, src, 4);
        FMISS_DEBUG("fmiss_vm: r%d <- *0x%08x (0x%08x) & 0x%08x\n", dst_reg, src + 0x38A00000, val, imm);
        vm->regs[dst_reg] = val & imm;
        break;
    case 0x5: // Read immediate into register.
        FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (immediate)\n", dst_reg, imm);
        vm->regs[dst_reg] = imm;
        break;
    case 0x6: // simple mov
        FMISS_DEBUG("fmiss_vm: r%d <- r%d (0x%08x)\n", dst_reg, src_reg, vm->regs[src_reg]);
        vm->regs[dst_reg] = vm->regs[src_reg];
        break;
    case 0x7: // Unknown, possibly wait for FMCSTAT. No-op.
        FMISS_DEBUG("fmiss_vm: pretending to wait for bit %d in status\n", dst);
        break;
    case 0xa: // AND two registers and an immediate.
        if (imm == 0) {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) &= r%d (0x%08x))\n", dst_reg, vm->regs[dst_reg] & vm->regs[src_reg], dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg]);
            vm->regs[dst_reg] &= vm->regs[src_reg];
            break;
        } else {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) = r%d (0x%08x) & 0x%08x)\n", dst_reg, vm->regs[src_reg] & imm, dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg], imm);
            vm->regs[dst_reg] = vm->regs[src_reg] & imm;
            break;
        }
        break;
    case 0x0b: // OR two registers and an immediate.
        if (imm == 0) {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) |= r%d (0x%08x))\n", dst_reg, vm->regs[dst_reg] | vm->regs[src_reg], dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg]);
            vm->regs[dst_reg] |= vm->regs[src_reg];
            break;
        } else {
            vm->regs[dst_reg] = vm->regs[src_reg] | imm;
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) = r%d (0x%08x) | 0x%08x)\n", dst_reg, vm->regs[src_reg] | imm, dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg], imm);
            break;
        }
        break;
    case 0x0c: // Add an Immediate to a Register
        if(imm == 0) {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) + r%d (0x%08x))\n", dst_reg, vm->regs[src_reg] + vm->regs[dst_reg], dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg]);
            vm->regs[dst_reg] += vm->regs[src_reg];
        } else {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) + 0x%08x)\n", dst_reg, vm->regs[src_reg] + imm, src_reg, vm->regs[src_reg], imm);
            vm->regs[dst_reg] = vm->regs[src_reg] + imm;
        }
        break;
    case 0x0d: // Subtract an Immediate from a Register
        if(imm == 0) {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) - r%d (0x%08x))\n", dst_reg, vm->regs[src_reg] - vm->regs[dst_reg], dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg]);
            vm->regs[dst_reg] -= vm->regs[src_reg];
        } else {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) - 0x%08x)\n", dst_reg, vm->regs[src_reg] - imm, src_reg, vm->regs[src_reg], imm);
            vm->regs[dst_reg] = vm->regs[src_reg] - imm;
        }
        break;
    case 0x0e: // Jump If Not Zero.
        FMISS_DEBUG("fmiss_vm: jnz r%d (0x%08x), 0x%08x (jump %staken)\n", dst_reg, vm->regs[dst_reg], imm, vm->regs[dst_reg] != 0 ? "" : "not ");
        if (vm->regs[dst_reg] != 0) {
            vm->pc = vm->start_pc + imm;
            return true;
        }
        break;
    case 0x11: // Store a Register Value to a Memory Location Pointed to by a Register.
        uint32_t addr = vm->regs[src_reg];
        uint32_t data = vm->regs[dst_reg];
        FMISS_DEBUG("fmiss_vm: *r%d (0x%08x) <- r%d (0x%08x)\n", src_reg, addr, dst_reg, data);
        address_space_write(vm->iomem, addr ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, &data, 4);
        break;
    case 0x13: // Left Shift a Register by an Immediate
        if(imm == 0) {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) << r%d (0x%08x))\n", dst_reg, vm->regs[src_reg] << vm->regs[dst_reg], dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg]);
            vm->regs[dst_reg] <<= vm->regs[src_reg];
        } else {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) << 0x%08x)\n", dst_reg, vm->regs[src_reg] << imm, src_reg, vm->regs[src_reg], imm);
            vm->regs[dst_reg] = vm->regs[src_reg] << imm;
        }
        break;
    case 0x14: // Right Shift a Register by an Immediate
        if(imm == 0) {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) >> r%d (0x%08x))\n", dst_reg, vm->regs[src_reg] >> vm->regs[dst_reg], dst_reg, vm->regs[dst_reg], src_reg, vm->regs[src_reg]);
            vm->regs[dst_reg] >>= vm->regs[src_reg];
        } else {
            FMISS_DEBUG("fmiss_vm: r%d <- 0x%08x (r%d (0x%08x) >> 0x%08x)\n", dst_reg, vm->regs[src_reg] >> imm, src_reg, vm->regs[src_reg], imm);
            vm->regs[dst_reg] = vm->regs[src_reg] >> imm;
        }
        break;
    case 0x17: // Jump if zero
        FMISS_DEBUG("fmiss_vm: jz r%d (0x%08x), 0x%08x (jump %staken)\n", dst_reg, vm->regs[dst_reg], imm, vm->regs[dst_reg] == 0 ? "" : "not ");
        if (vm->regs[dst_reg] == 0) {
            vm->pc = vm->start_pc + imm;
            return true;
        }
        break;
    case 0x18: // Load Register Value from DMA Memory Given by Offset in a Register
        uint32_t value = nand_mem_read(opaque, vm->regs[src_reg], 4);
        FMISS_DEBUG("fmiss_vm: r%d <- *r%d (0x%08x <- 0x%08x)\n", dst_reg, src, vm->regs[src_reg] + 0x38A00000, value);
        vm->regs[dst_reg] = value;
        break;
    case 0x19:
        FMISS_DEBUG("fmiss_vm: *r%d <- r%d (0x%08x <- 0x%08x)\n", src_reg, dst_reg, vm->regs[src_reg] + 0x38A00000, vm->regs[dst_reg]);
        nand_mem_write(opaque, vm->regs[src_reg], vm->regs[dst_reg], 4);
        break;
    default:
        hw_error("fmiss_vm: unimplemented opcode 0x%02X!\n", opcode);
        return false;
    }
    vm->pc += 8;
    return true;
}

static void fmiss_vm_execute(void *opaque, fmiss_vm *vm) {
    printf("fmiss_vm: starting execution at %08lx...\n", vm->start_pc);
    while (fmiss_vm_step(opaque, vm)) {
    }
    printf("fmiss_vm: done executing\n");
}

static int get_bank(NandState *s) {
    uint32_t bank = __builtin_ctz(s->fmctrl0 >> 1);
    printf("fmctrl0: 0x%08x (bank %d selected)\n", s->fmctrl0, bank);
    if(bank > 7) return -1;
    return bank;
}

static void set_bank(NandState *s, uint32_t activate_bank) {
    for(int bank = 0; bank < 8; bank++) {
        // clear bit, toggle if it is active
        s->fmctrl0 &= ~(1 << (bank + 1));
        if(bank == activate_bank) {
            s->fmctrl0 ^= 1 << (bank + 1);
        }
    }
}

void nand_set_buffered_page(NandState *s, uint32_t page) {
    uint32_t bank = get_bank(s);
    if(bank == -1) {
        printf("Active bank not set while nand_read with page %d is called (reading multiple pages: %d)!\n", page, s->reading_multiple_pages);
    }

    if(bank != s->buffered_bank || page != s->buffered_page) {
        // refresh the buffered page
        //uint32_t vpn = page * 8 + bank;
        char filename[200];
        sprintf(filename, "%s/bank%d/%d.page", s->nand_path, bank, page);
        struct stat st = {0};
        if (stat(filename, &st) == -1) {
            // page storage does not exist - initialize an empty buffer
            memset(s->page_buffer, 0, NAND_BYTES_PER_PAGE);
            memset(s->page_spare_buffer, 0, NAND_BYTES_PER_SPARE);
            s->page_spare_buffer[0xA] = 0xFF; // make sure we add the FTL mark to an empty page
        }
        else {
            FILE *f = fopen(filename, "rb");
            if (f == NULL) { hw_error("Unable to read file!"); }
            fread(s->page_buffer, sizeof(char), NAND_BYTES_PER_PAGE, f);
            fread(s->page_spare_buffer, sizeof(char), NAND_BYTES_PER_SPARE, f);
            fclose(f);
        }

        s->buffered_page = page;
        s->buffered_bank = bank;
        // printf("Buffered bank: %d, page: %d\n", s->buffered_bank, s->buffered_page);
    }
}

static uint64_t nand_mem_read(void *opaque, hwaddr addr, unsigned size) {
    NandState *s = (NandState *) opaque;
    //printf("%s(%08lx)...\n", __func__, addr);
    if(s->reading_multiple_pages) {
        fprintf(stderr, "%s: reading from 0x%08lx\n", __func__, addr);
    }

    if (addr >= FMI_DMEM && addr < (FMI_DMEM+4*FMIVSS_DMEM_SIZE)) {
        uint32_t i = (addr - FMI_DMEM)/4;
        return s->fmiss_vm.dmem[i];
    }

    switch (addr) {
        case NAND_FMCTRL0:
            printf("Reading from FMCTRL0: %08X\n", s->fmctrl0);
            return s->fmctrl0;
        case NAND_FMCTRL1:
            return s->fmctrl1;
        case NAND_FMFIFO:
            printf("Reading from FMFIFO, cmd: %02x\n", s->cmd);
            if(s->cmd == NAND_CMD_ID) {
                printf("FIFO Reading ID from bank %d, will return %x\n", get_bank(s), (get_bank(s) < NAND_NUM_BANKS_INSTALLED ? NAND_CHIP_ID : 0));
                if(get_bank(s) < NAND_NUM_BANKS_INSTALLED) return NAND_CHIP_ID;
                else return 0;
            }
            else if(s->cmd == NAND_CMD_READSTATUS) {
                return (1 << 6);
            }
            else {
                printf("Reading from FMFIFO while not in ID or READSTATUS mode!\n");
                uint32_t read_val = 0;
                if(s->reading_multiple_pages) {
                    printf("Reading multiple pages, fmdnum: %d\n", s->fmdnum);
                    // which bank are we at?
                    if(s->fmdnum % 0x800 == 0) {
                        s->cur_bank_reading += 1;
                        printf("WILL TURN TO BANK %d (cnt: %d)\n", s->cur_bank_reading, s->fmdnum);
                        set_bank(s, s->banks_to_read[s->cur_bank_reading]);
                    }

                    // compute the offset in the page
                    uint32_t page_offset = s->fmdnum % 0x800;
                    if(page_offset == 0) { page_offset = 0x800; }
                    nand_set_buffered_page(s, s->pages_to_read[s->cur_bank_reading]);
                    printf("Reading bank %d page %d\n", get_bank(s), s->pages_to_read[s->cur_bank_reading]);
                    read_val = ((uint32_t *)s->page_buffer)[(NAND_BYTES_PER_PAGE - page_offset) / 4];
                    printf("FMDNUM: %d, offset: %d\n", s->fmdnum, (NAND_BYTES_PER_PAGE - page_offset) / 4);
                    printf("Page offset: %d, bytes: 0x%08x\n", page_offset, read_val);
                } else {
                    uint32_t page = (s->fmaddr1 << 16) | (s->fmaddr0 >> 16);
                    nand_set_buffered_page(s, page);
                    printf("Reading bank %d page 0x%X to 0x%08X, fmctrl0: %x\n", get_bank(s), page, s->destaddr, s->fmctrl0);

                    // read the data
                    void* buffer = malloc(NAND_BYTES_PER_PAGE);
                    cow_read(s->nand_banks[get_bank(s)], buffer, page * NAND_BYTES_PER_PAGE, NAND_BYTES_PER_PAGE);
                    address_space_write(s->downstream_as, s->destaddr ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, buffer, NAND_BYTES_PER_PAGE);
                    free(buffer);

                    // read the spare data
                    cow_read(s->nand_spares[get_bank(s)], s->spare_buffer, page * 16, 12);
                }
                s->fmdnum -= 4;
                printf("FIFO[0] read: 0x%08x\n", s->spare_buffer[0]);
                return s->spare_buffer[0];
            }
        case 0x64:
            printf("FIFO[1] read: 0x%08x\n", s->spare_buffer[1]);
            return s->spare_buffer[1]; // "reserved" field in the VFLSpare struct
        case 0x68:
            printf("FIFO[2] read: 0x%08x\n", s->spare_buffer[2]);
            return s->spare_buffer[2]; // third byte is the VFLSpare Sparetype (0x80 for now), the fourth byte is the StatusMark. the other two bytes are unused.

        case 0x80:
            if (s->cmd == NAND_CMD_ID) {
                printf("0x80 Reading ID from bank %d, will return %x\n", get_bank(s), (get_bank(s) < NAND_NUM_BANKS_INSTALLED ? NAND_CHIP_ID : 0));
                if(get_bank(s) < NAND_NUM_BANKS_INSTALLED) return NAND_CHIP_ID;
                else return 0;
            }
            return 0xdeadbeef;
        case NAND_FMCSTAT:
            return (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12); // this indicates that everything is ready, including our eight banks
        case NAND_RSCTRL:
            return s->rsctrl;
        case FMI_PROGRAM:
            return s->fmi_program;
        case FMI_INT:
            printf("FMI_INT: 0x%08x\n", s->fmi_int);
            return s->fmi_int;
        case FMI_START:
            return 0;
        // case 0xC18 ... 0xC34:
        //     return s->fmiss_vm.regs[(addr - 0xC18) / 4];
        case 0xc64:
            return 1;
        default:
            break;
    }
    printf("defaulting %X to 0\n", addr);
    return 0;
}

static void nand_mem_write(void *opaque, hwaddr addr, uint64_t val, unsigned size) {
    printf("%s(%08lx, %08lx)...\n", __func__, addr, val);
    NandState *s = (NandState *) opaque;
    if(s->reading_multiple_pages) {
        fprintf(stderr, "%s: writing 0x%08lx to 0x%08lx\n", __func__, val, addr);
    }

    if (addr >= FMI_DMEM && addr < (FMI_DMEM+4*FMIVSS_DMEM_SIZE)) {
        uint32_t i = (addr - FMI_DMEM)/4;
        s->fmiss_vm.dmem[i] = val;
        return;
    }
    
    switch(addr) {
        case NAND_FMCTRL0:
            s->fmctrl0 = val;
            printf("NAND_FMCTRL0: 0x%08x\n", s->fmctrl0);
            break;
        case NAND_FMCTRL1:
            s->fmctrl1 = val | (1<<30);
            printf("NAND_FMCTRL1: 0x%08x\n", s->fmctrl1);
            break;
        case NAND_FMADDR0:
            s->fmaddr0 = val;
            break;
        case NAND_FMADDR1:
            s->fmaddr1 = val;
            break;
        case NAND_FMANUM:
            s->fmanum = val;
            break;
        case NAND_CMD:
            s->cmd = val;
            break;
        case NAND_FMDNUM:
            printf("NAND_FMDNUM Written! 0x%08x\n", val);
            if(val == NAND_BYTES_PER_SPARE - 1) {
                s->reading_spare = 1;
            } else {
                s->reading_spare = 0;
            }
            s->fmdnum = val;
            break;
        case NAND_FMFIFO:
            if(!s->is_writing) {
                // printf("%s: NAND_FMFIFO writing while not in writing mode!\n", __func__);
                return;
            }

            //printf("Setting offset %d: %d\n", s->fmdnum, (NAND_BYTES_PER_PAGE - s->fmdnum) / 4);
            ((uint32_t *)s->page_buffer)[(NAND_BYTES_PER_PAGE - s->fmdnum) / 4] = val;
            s->fmdnum -= 4;

            if(s->fmdnum == 0) {
                // we're done!
                s->is_writing = false;

                // flush the page buffer to the disk
                //uint32_t vpn = s->buffered_page * 8 + s->buffered_bank;
                //printf("Flushing page %d, bank %d, vpn %d\n", s->buffered_page, s->buffered_bank, vpn);
                qemu_mutex_lock(&s->lock);
                qemu_mutex_unlock(&s->lock);
                {
                    char filename[200];
                    sprintf(filename, "%s/bank%d/%d_new.page", s->nand_path, s->buffered_bank, s->buffered_page);
                    FILE *f = fopen(filename, "wb");
                    if (f == NULL) { hw_error("Unable to read file!"); }
                    fwrite(s->page_buffer, sizeof(char), NAND_BYTES_PER_PAGE, f);
                    fwrite(s->page_spare_buffer, sizeof(char), NAND_BYTES_PER_SPARE, f);
                    fclose(f);
                }
            }
            break;
        case NAND_DESTADDR:
            s->destaddr = val;
            printf("DESTADDR SET, DUMP OF STATE: \n");
            printf("fmctrl0: 0x%08x\n", s->fmctrl0);
            printf("fmctrl1: 0x%08x\n", s->fmctrl1);
            printf("fmaddr0: 0x%08x\n", s->fmaddr0);
            printf("fmaddr1: 0x%08x\n", s->fmaddr1);
            printf("fmanum: 0x%08x\n", s->fmanum);
            printf("cmd: 0x%08x\n", s->cmd);
            printf("fmdnum: 0x%08x\n", s->fmdnum);
            printf("destaddr: 0x%08x\n", s->destaddr);
            printf("rsctrl: 0x%08x\n", s->rsctrl);
            break;
        case NAND_RSCTRL:
            s->rsctrl = val;
            break;
        case FMI_PROGRAM:
            s->fmi_program = val;
            break;
        case FMI_INT:
            s->fmi_int &= ~val;
            break;
        case FMI_START:
            if (val == 0xfff5) {
                s->fmiss_vm.iomem = s->downstream_as;
                fmiss_vm_reset(&s->fmiss_vm, s->fmi_program);
                fmiss_vm_execute(opaque, &s->fmiss_vm);
                qemu_irq_raise(s->irq);
                s->fmi_int |= 1;
            } else {
            }
            break;
        default:
            break;
    }
}

static const MemoryRegionOps nand_ops = {
    .read = nand_mem_read,
    .write = nand_mem_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void nand_init(Object *obj) {
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    NandState *s = IPOD_NANO3G_NAND(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &nand_ops, s, "nand", 0x1000);
    sysbus_init_irq(sbd, &s->irq);

    s->page_buffer = (uint8_t *)malloc(NAND_BYTES_PER_PAGE);
    s->page_spare_buffer = (uint8_t *)malloc(NAND_BYTES_PER_SPARE);
    s->buffered_page = -1;
    s->buffered_bank = -1;

    fmiss_vm_reset(&s->fmiss_vm, 0);

    s->nand_banks = (cow_file**) malloc(sizeof(cow_file*) * NAND_NUM_BANKS);
    s->nand_banks[0] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank0.bin");
    s->nand_banks[1] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank1.bin");
    s->nand_banks[2] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank2.bin");
    s->nand_banks[3] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank3.bin");

    s->nand_spares = (cow_file**) malloc(sizeof(cow_file*) * NAND_NUM_BANKS);
    s->nand_spares[0] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank0-spare.bin");
    s->nand_spares[1] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank1-spare.bin");
    s->nand_spares[2] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank2-spare.bin");
    s->nand_spares[3] = cow_open("/mnt/hgfs/ipodmemory/nand-dump-bank3-spare.bin");
    memset(s->spare_buffer, 0xff, 3*sizeof(uint32_t));

    qemu_mutex_init(&s->lock);
}

static void nand_reset(DeviceState *d) {
    NandState *s = (NandState *) d;

    s->fmctrl0 = 0;
    s->fmctrl1 = 0;
    s->fmaddr0 = 0;
    s->fmaddr1 = 0;
    s->fmanum = 0;
    s->fmdnum = 0;
    s->rsctrl = 0;
    s->cmd = 0;
    s->fmi_program = 0;
    s->fmi_int = 0;
    s->reading_spare = 0;
    s->buffered_page = -1;
}

static void nand_class_init(ObjectClass *oc, void *data) {
    DeviceClass *dc = DEVICE_CLASS(oc);
    dc->reset = nand_reset;
}

static const TypeInfo nand_info = {
    .name          = TYPE_IPOD_NANO3G_NAND,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NandState),
    .instance_init = nand_init,
    .class_init    = nand_class_init,
};

static void nand_register_types(void) {
    type_register_static(&nand_info);
}

type_init(nand_register_types)
