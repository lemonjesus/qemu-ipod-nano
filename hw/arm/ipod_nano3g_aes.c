#include "hw/arm/ipod_nano3g_aes.h"

static uint64_t S5L8702_aes_read(void *opaque, hwaddr offset, unsigned size)
{
    struct S5L8702AESState *aesop = (struct S5L8702AESState *)opaque;

    switch(offset) {
        case AES_STATUS:
            return aesop->status;
      default:
            //fprintf(stderr, "%s: UNMAPPED AES_ADDR @ offset 0x%08x\n", __FUNCTION__, offset);
            break;
    }

    return 0;
}

static void S5L8702_aes_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    struct S5L8702AESState *aesop = (struct S5L8702AESState *)opaque;
    static uint8_t keylenop = 0;

    uint8_t *inbuf;
    uint8_t *buf;

    // fprintf(stderr, "%s: offset 0x%08x value 0x%08x\n", __FUNCTION__, offset, value);

    switch(offset) {
        case AES_GO:
            inbuf = (uint8_t *)malloc(aesop->insize);
            cpu_physical_memory_read((aesop->inaddr), inbuf, aesop->insize);

            switch(aesop->keytype) {
                    case AESGID:    
                        fprintf(stderr, "%s: No support for GID key\n", __func__);
                        break;         
                    case AESUID:
                        AES_set_decrypt_key(key_uid, sizeof(key_uid) * 8, &aesop->decryptKey);
                        break;
                    case AESCustom:
                        AES_set_decrypt_key((uint8_t *)aesop->custkey, 0x20 * 8, &aesop->decryptKey);
                        break;
            }

            buf = (uint8_t *) malloc(aesop->insize);

            if(aesop->keytype != 0x01) AES_cbc_encrypt(inbuf, buf, aesop->insize, &aesop->decryptKey, (uint8_t *)aesop->ivec, aesop->operation);

            printf("AES: %s %d bytes from 0x%08x to 0x%08x\n", aesop->operation == AES_DECRYPT ? "decrypted" : "encrypted", aesop->insize, aesop->inaddr, aesop->outaddr);

            cpu_physical_memory_write((aesop->outaddr), buf, aesop->insize);
            memset(aesop->custkey, 0, 0x20);
            memset(aesop->ivec, 0, 0x10);
            free(inbuf);
            free(buf);
            keylenop = 0;
            aesop->outsize = aesop->insize;
            aesop->status = 0xf;
            break;
        case AES_KEYLEN:
            if(keylenop == 1) {
                aesop->operation = value;
            }
            keylenop++;
            aesop->keylen = value;
            break;
        case AES_INADDR:
            aesop->inaddr = value;
            break;
        case AES_INSIZE:
            aesop->insize = value;
            break;
        case AES_OUTSIZE:
            aesop->outsize = value;
            break;
        case AES_OUTADDR:
            aesop->outaddr = value;
            break;
        case AES_TYPE:
            aesop->keytype = value;
            break;
        case AES_KEY_REG ... ((AES_KEY_REG + AES_KEYSIZE) - 1):
            {
                uint8_t idx = (offset - AES_KEY_REG) / 4;
                aesop->custkey[idx] |= value;
                break;
            }
        case AES_IV_REG ... ((AES_IV_REG + AES_IVSIZE) -1 ):
            {
                uint8_t idx = (offset - AES_IV_REG) / 4;
                aesop->ivec[idx] |= value;
                break;
            }
        default:
            //fprintf(stderr, "%s: UNMAPPED AES_ADDR @ offset 0x%08x - 0x%08x\n", __FUNCTION__, offset, value);
            break;
    }
}

static const MemoryRegionOps aes_ops = {
    .read = S5L8702_aes_read,
    .write = S5L8702_aes_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void S5L8702_aes_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    DeviceState *dev = DEVICE(sbd);
    S5L8702AESState *s = IPOD_NANO3G_AES(dev);

    memory_region_init_io(&s->iomem, obj, &aes_ops, s, "aes", 0x100);
    sysbus_init_mmio(sbd, &s->iomem);

    memset(&s->custkey, 0, 8 * sizeof(uint32_t));
    memset(&s->ivec, 0, 4 * sizeof(uint32_t));
}

static void S5L8702_aes_class_init(ObjectClass *klass, void *data)
{

}

static const TypeInfo ipod_nano3g_aes_info = {
    .name          = TYPE_IPOD_NANO3G_AES,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(S5L8702AESState),
    .instance_init = S5L8702_aes_init,
    .class_init    = S5L8702_aes_class_init,
};

static void ipod_nano3g_machine_types(void)
{
    type_register_static(&ipod_nano3g_aes_info);
}

type_init(ipod_nano3g_machine_types)