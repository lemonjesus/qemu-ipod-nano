#include "hw/arm/ipod_nano3g_jpeg.h"

uint8_t zigzag[] = { 0, 1, 5, 6, 14, 15, 27, 28, 
                     2, 4, 7, 13, 16, 26, 29, 42, 
                     3, 8, 12, 17, 25, 30, 41, 43, 
                     9, 11, 18, 24, 31, 40, 44, 53, 
                     10, 19, 23, 32, 39, 45, 52, 54, 
                     20, 22, 33, 38, 46, 51, 55, 60, 
                     21, 34, 37, 47, 50, 56, 59, 61, 
                     35, 36, 48, 49, 57, 58, 62, 63 };

static uint8_t clamp(double x) {
    if (x < 0) {
        return 0;
    } else if (x > 255) {
        return 255;
    } else {
        return x;
    }
}

static double idct_lookup[8][8][8][8];

static void s5l8702_jpeg_decode(EncodedMCU* mcu, uint32_t* qtable1, uint32_t* qtable2, uint8_t* yout, uint8_t* cbout, uint8_t* crout) {
    DecodedMCU* decoded = malloc(sizeof(DecodedMCU) * 300);
    double *yplane = malloc(sizeof(double) * 320 * 240);
    double *crplane = malloc(sizeof(double) * 320 * 240);
    double *cbplane = malloc(sizeof(double) * 320 * 240);

    int32_t dct_matrix[8][8];

    for(int i = 0; i < 300; i++) {
        // do the luminance blocks
        for (int data_unit_row = 0; data_unit_row < 2; data_unit_row++) {
            for (int data_unit_column = 0; data_unit_column < 2; data_unit_column++) {
                
                // dequantize
                for (int l = 0; l < 0x40; l++) {
                    dct_matrix[l / 8][l % 8] = __builtin_bswap32(mcu[i].lum[data_unit_row*2+data_unit_column].coeff[zigzag[l]]) * qtable1[l];
                }

                // perform IDCT
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        double sum = 0;
                        for (int u = 0; u < 8; u++) {
                            for (int v = 0; v < 8; v++) {
                                sum += idct_lookup[y][x][u][v] * dct_matrix[u][v];
                            }
                        }

                        decoded[i].yplane[data_unit_column*8+y][data_unit_row*8+x] = round(sum/4 + 128);
                    }
                }
            }
        }

        // do the chrominance blocks
        // dequantize
        for (int l = 0; l < 0x40; l++) {
            dct_matrix[l / 8][l % 8] = __builtin_bswap32(mcu[i].chromb.coeff[zigzag[l]]) * qtable2[l];
        }

        // perform IDCT
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                double sum = 0;
                for (int u = 0; u < 8; u++) {
                    for (int v = 0; v < 8; v++) {
                        sum += idct_lookup[y][x][u][v] * dct_matrix[u][v];
                    }
                }

                // set four pixels
                decoded[i].cbplane[y][x] = round(sum/4 + 128);
            }
        }

        // dequantize
        for (int l = 0; l < 0x40; l++) {
            dct_matrix[l / 8][l % 8] = __builtin_bswap32(mcu[i].chromr.coeff[zigzag[l]]) * qtable2[l];
        }

        // perform IDCT
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                double sum = 0;
                for (int u = 0; u < 8; u++) {
                    for (int v = 0; v < 8; v++) {
                        sum += idct_lookup[y][x][u][v] * dct_matrix[u][v];
                    }
                }

                // set four pixels
                decoded[i].crplane[y][x] = round(sum/4 + 128);
            }
        }
    }

    // reconstruct the image
    for (int i = 0; i < 300; i++) {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                yplane[(i%20)*16 + (i/20) * 16 * 320 + (y)*320 + x] = decoded[i].yplane[x][y];
            }
        }
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                cbplane[(i%20)*8 + (i/20) * 8 * 160 + (y)*160 + x] = decoded[i].cbplane[x][y];
                crplane[(i%20)*8 + (i/20) * 8 * 160 + (y)*160 + x] = decoded[i].crplane[x][y];
            }
        }
    }

    for (int i = 0; i < 320 * 240; i++) {
        yout[i] = clamp(yplane[i]);
    }

    for (int i = 0; i < 160 * 120; i++) {
        cbout[i] = clamp(cbplane[i]);
        crout[i] = clamp(crplane[i]);
    }

    for (int i = 0; i < 320 * 240; i += 4) {
        // __builtin_bswap32 the pixels to reverse columns of 4 pixels in place in each channel
        uint32_t *y32 = (uint32_t *)&yout[i];
        uint32_t *cb32 = (uint32_t *)&cbout[i];
        uint32_t *cr32 = (uint32_t *)&crout[i];

        *y32 = __builtin_bswap32(*y32);
        *cb32 = __builtin_bswap32(*cb32);
        *cr32 = __builtin_bswap32(*cr32);
    }

    free(decoded);
    free(yplane);
    free(cbplane);
    free(crplane);
}

static uint64_t s5l8702_jpeg_read(void *opaque, hwaddr addr, unsigned size) {
    S5L8702JPEGState *s = S5L8702JPEG(opaque);

    uint32_t r = 0;

    switch (addr) {
    case 0x60000:
        r = 0xFFFFFFFF;
        break;
    
    default:
        break;
    }

    // fprintf(stderr, "%s: reading 0x%08x -> 0x%08x\n", __func__, addr, r);
    
    return r;
}

static void s5l8702_jpeg_write(void *opaque, hwaddr addr, uint64_t data, unsigned size) {
    // fprintf(stderr, "%s: writing 0x%08x to 0x%08x\n", __func__, data, addr);
    
    S5L8702JPEGState *s = S5L8702JPEG(opaque);
    s->regs[addr / 4] = data;

    switch (addr) {
    case JPEG_REG_QTABLE1 ... JPEG_REG_QTABLE1 + JPEG_QTABLE_LEN:
        s->qtable1[(addr - JPEG_REG_QTABLE1)/4] = data;
        break;
    case JPEG_REG_QTABLE2 ... JPEG_REG_QTABLE2 + JPEG_QTABLE_LEN:
        s->qtable2[(addr - JPEG_REG_QTABLE2)/4] = data;
        break;
    case JPEG_REG_CTRL:
        EncodedMCU* mcu = malloc(sizeof(EncodedMCU) * 300);
        uint8_t *yplane = malloc(sizeof(uint8_t) * 320 * 240);
        uint8_t *cbplane = malloc(sizeof(uint8_t) * 320 * 240);
        uint8_t *crplane = malloc(sizeof(uint8_t) * 320 * 240);

        MemTxResult result = address_space_read(s->nsas, s->regs[JPEG_REG_COEFF_BLOCKS/4] ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, mcu, sizeof(EncodedMCU) * 300);

        s5l8702_jpeg_decode(mcu, s->qtable1, s->qtable2, yplane, cbplane, crplane);

        address_space_write(s->nsas, (s->regs[JPEG_REG_OUT_CRPLANE/4] + 0x10000) ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, yplane, 320 * 240);
        address_space_write(s->nsas, (s->regs[JPEG_REG_OUT_CRPLANE/4] + 0x10000 + 0x12C00) ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, cbplane, 160 * 120);
        address_space_write(s->nsas, (s->regs[JPEG_REG_OUT_CRPLANE/4] + 0x10000 + 0x12C00 + 0x4B00) ^ 0x80000000, MEMTXATTRS_UNSPECIFIED, crplane, 160 * 120);

        free(mcu);
        free(yplane);
        free(cbplane);
        free(crplane);
        break;
    
    default:
        break;
    }
}

static const MemoryRegionOps jpeg_ops = {
    .read = s5l8702_jpeg_read,
    .write = s5l8702_jpeg_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void s5l8702_jpeg_reset(DeviceState *d) {
    S5L8702JPEGState *s = (S5L8702JPEGState *)d;
	memset(s->regs, 0, sizeof(s->regs));
}

static void s5l8702_jpeg_realize(DeviceState *dev, struct Error **errp) {
    S5L8702JPEGState *s = S5L8702JPEG(dev);

    // precompute the IDCT lookup table
    for (int y = 0; y < 8; y++) { 
        for (int x = 0; x < 8; x++) { 
            for (int u = 0; u < 8; u++) { 
                for (int v = 0; v < 8; v++) { 
                    double cu = (u == 0) ? (1 / sqrt(2)) : 1;
                    double cv = (v == 0) ? (1 / sqrt(2)) : 1;
                    idct_lookup[y][x][u][v] = cu * cv * cos(((2 * x + 1) * u * M_PI) / 16) * cos(((2 * y + 1) * v * M_PI) / 16);
                }
            }
        }
    }
}

static void s5l8702_jpeg_init(Object *obj) {
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    S5L8702JPEGState *s = S5L8702JPEG(dev);

    memory_region_init_io(&s->iomem, obj, &jpeg_ops, s, "jpeg", sizeof(s->regs));
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);

    s5l8702_jpeg_reset(s);
}

static void s5l8702_jpeg_class_init(ObjectClass *klass, void *data) {
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = s5l8702_jpeg_realize;
    dc->reset = s5l8702_jpeg_reset;
}

static const TypeInfo s5l8702_jpeg_info = {
    .name          = TYPE_S5L8702JPEG,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S5L8702JPEGState),
    .instance_init = s5l8702_jpeg_init,
    .class_init    = s5l8702_jpeg_class_init,
};

static void s5l8702_jpeg_register_types(void)
{
    type_register_static(&s5l8702_jpeg_info);
}

type_init(s5l8702_jpeg_register_types)