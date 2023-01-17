## How to get the files required to run the emulator:
### BOOTROM
Simply dump this with wInd3x from a real iPod Nano 3G. The emulator will make the required patches to get around the bootrom's security checks. The SHA1 of the bootrom is `6de08ee9e89d5a4ef59b629c903d4ceb828030f0`.

### NOR and EFI Bootloader
Given a full NOR dump and a decrypted EFI image, you can use:

```
./build_nor.sh nor_dump.bin /home/tucker/Development/n3g-emulator/efi.bin nor_image.bin
```

### NAND
TODO

## Other Notes
Run it with:
```
./arm-softmmu/qemu-system-arm -M iPod-Touch,bootrom=/home/tucker/Development/qemu-ipod-nano/build/s5l8702-bootrom.bin,iboot=/home/tucker/Development/qemu-ipod-nano/build/iboot_204_n45ap.bin,nand=/home/tucker/Development/qemu-ipod-nano/build/nand -serial mon:stdio -cpu max -m 1G -d unimp -pflash /home/tucker/Development/qemu-ipod-nano/build/nor_image.bin
```

Run the iPhone code with:
```
./arm-softmmu/qemu-system-arm -M iPod-Touch,bootrom=/home/tucker/Development/qemu-ipod-nano/build/bootrom_s5l8900,iboot=/home/tucker/Development/qemu-ipod-nano/build/iboot_204_n45ap.bin,nand=/home/tucker/Development/qemu-ipod-nano/build/nand -serial mon:stdio -cpu max -m 1G -d unimp -pflash /home/tucker/Development/qemu-ipod-nano/build/nor_n45ap.bin
```

Configure for finding strange malloc bugs:

```
../configure --enable-sdl --disable-cocoa --target-list=arm-softmmu --disable-capstone --disable-pie --disable-slirp --disable-werror --extra-cflags="-g" --extra-ldflags='-lcrypto'
```
