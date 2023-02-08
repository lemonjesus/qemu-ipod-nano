#ifndef IPOD_NANO3G_NOR_SPI_H
#define IPOD_NANO3G_NOR_SPI_H

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/timer.h"
#include "hw/ssi/ssi.h"
#include "hw/hw.h"

#define TYPE_IPOD_NANO3G_NOR_SPI                "ipodnano3g.norspi"
OBJECT_DECLARE_SIMPLE_TYPE(IPodNano3GNORSPIState, IPOD_NANO3G_NOR_SPI)

#define NOR_READ_DATA_CMD  0x3
#define NOR_GET_STATUS_CMD 0x5
#define NOR_ENABLE_STATUS_WRITE_CMD 0x50
#define NOR_WRITE_STATUS_CMD 0x1
#define NOR_RESET_CMD 0xFF

typedef struct IPodNano3GNORSPIState {
    SSIPeripheral ssidev;
    uint32_t cur_cmd;
    uint8_t *in_buf;
    uint8_t *out_buf;
    uint32_t in_buf_size;
    uint32_t out_buf_size;
    uint32_t in_buf_cur_ind;
    uint32_t out_buf_cur_ind;
    uint8_t *nor_data;
    uint32_t nor_read_ind;
    char nor_path[1024];
    bool nor_initialized;
} IPodNano3GNORSPIState;

#endif