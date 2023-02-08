#include "hw/arm/ipod_nano3g_nor_spi.h"

static void initialize_nor(IPodNano3GNORSPIState *s)
{
    unsigned long fsize;
    // TODO still hardcoded, string copy not working...
    if (g_file_get_contents("/home/tucker/Development/qemu-ipod-nano/build/nor_image.bin", (char **)&s->nor_data, &fsize, NULL)) {
        s->nor_initialized = true;
    }
}

static uint32_t ipod_nano3g_nor_spi_transfer(SSIPeripheral *dev, uint32_t value)
{
    IPodNano3GNORSPIState *s = IPOD_NANO3G_NOR_SPI(dev);

    // if(value != 0xFF) printf("NOR SPI received value 0x%08x\n", value);

    if(s->cur_cmd == NOR_READ_DATA_CMD && s->in_buf_cur_ind == s->in_buf_size && value != 0xFF) {
        // if we are currently reading from the NOR data and we receive a value that's not the sentinel, reset the current command.
        s->cur_cmd = 0;
    }

    if(s->cur_cmd == 0) {
        // this is a new command -> set it
        s->cur_cmd = value;
        s->out_buf = malloc(0x1000);
        s->in_buf = malloc(0x100);
        s->in_buf[0] = value;
        s->in_buf_size = 0;
        s->in_buf_cur_ind = 1;
        s->out_buf_cur_ind = 0;

        switch(value) {
            case 0x00:
                break;
            case NOR_GET_STATUS_CMD:
                s->in_buf_size = 1;
                s->out_buf_size = 1;
                break;
            case NOR_READ_DATA_CMD:
                s->in_buf_size = 4;
                s->out_buf_size = 4096;
                break;
            case NOR_RESET_CMD:
                s->in_buf_size = 1;
                s->out_buf_size = 1;
                break;
            case NOR_ENABLE_STATUS_WRITE_CMD:
                s->in_buf_size = 1;
                s->out_buf_size = 1;
                break;
            case NOR_WRITE_STATUS_CMD:
                s->in_buf_size = 3;
                s->out_buf_size = 1;
                break;
            default:
                hw_error("Unknown command 0x%02x!", value);
        }

        return 0x0;
    }
    else if(s->cur_cmd != 0 && s->in_buf_cur_ind < s->in_buf_size) {
        // we're reading the command
        s->in_buf[s->in_buf_cur_ind] = value;
        s->in_buf_cur_ind++;

        if(s->cur_cmd == NOR_GET_STATUS_CMD && s->in_buf_cur_ind == s->in_buf_size) {
            s->out_buf[0] = 0x0;  // indicates that the NOR is ready
        } else if(s->cur_cmd == NOR_RESET_CMD && s->in_buf_cur_ind == s->in_buf_size) {
            s->out_buf[0] = 0x0;  // indicates that the NOR is reset
        } else if(s->cur_cmd == NOR_READ_DATA_CMD && s->in_buf_cur_ind == s->in_buf_size) {
            if(!s->nor_initialized) { initialize_nor(s); }
            s->nor_read_ind = (s->in_buf[1] << 16) | (s->in_buf[2] << 8) | s->in_buf[3];
            printf("Reading from NOR @ 0x%08X\n", s->nor_read_ind);
        }

        return 0x0;
    }
    else {
        uint8_t ret_val;
        // otherwise, we're outputting the response
        if(s->cur_cmd == NOR_READ_DATA_CMD) {
            uint8_t ret_val = s->nor_data[s->nor_read_ind];
            s->nor_read_ind++;
            return ret_val;
        }
        else {
            ret_val = s->out_buf[s->out_buf_cur_ind];
            s->out_buf_cur_ind++;

            if(s->cur_cmd != 0 && (s->out_buf_cur_ind == s->out_buf_size)) {
                // the command is done - clean up
                s->cur_cmd = 0;
                free(s->in_buf);
                free(s->out_buf);
            }
        }
        return ret_val;
    }
}

static void ipod_nano3g_nor_spi_realize(SSIPeripheral *d, Error **errp)
{
    IPodNano3GNORSPIState *s = IPOD_NANO3G_NOR_SPI(d);
    s->nor_initialized = 0;
}

static void ipod_nano3g_nor_spi_class_init(ObjectClass *klass, void *data)
{
    SSIPeripheralClass *k = SSI_PERIPHERAL_CLASS(klass);
    k->realize = ipod_nano3g_nor_spi_realize;
    k->transfer = ipod_nano3g_nor_spi_transfer;
}

static const TypeInfo ipod_nano3g_nor_spi_type_info = {
    .name = TYPE_IPOD_NANO3G_NOR_SPI,
    .parent = TYPE_SSI_PERIPHERAL,
    .instance_size = sizeof(IPodNano3GNORSPIState),
    .class_init = ipod_nano3g_nor_spi_class_init,
};

static void ipod_nano3g_nor_spi_register_types(void)
{
    type_register_static(&ipod_nano3g_nor_spi_type_info);
}

type_init(ipod_nano3g_nor_spi_register_types)
