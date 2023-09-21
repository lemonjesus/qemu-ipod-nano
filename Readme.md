# iPod Nano 3G Emulator

This project is an attempt at rehosting the entirety of the 3rd Generation iPod Nano. It is currently a work in progress, and is not yet functional. The goal is to be able to run the iPod Nano 3G's firmware in a QEMU emulator.

This work is based on devos50's work emulating the iPhone 1G and the iTouch 2G. See the repo this is forked from for more information.

Before I started this, I started a more bespoke emulator based on Unicorn. [Check that out here](https://github.com/lemonjesus/iPod-n3g-emulator), but it's by no means as far along as this project.

## What works

* The CPU, complete with system timers and interrupts
* SPI NOR flash
* I2C communications probably work, but the PMU is currently a dummy device
* The LCD screen
* Hardware JPEG decoding (it's good enough for now, but it isn't perfect)
* Basic clickwheel buttons
* NAND... kind of. It makes it through the initialization process, but that means the entire FMISS VM instruction set is basically proved to be correct.

Currently, the iPod Nano 3G emulator is able to boot partially all the way through the EFI bootloader. The BDS module decides to try and load Disk Mode for some reason, presumably because it can't load the OS (as far as I can tell, it doesn't even try?) My current goal is to get into the Diagnostic Menu because that's the simplest binary to load that proves that the base iPod is working.

Below this, the README is a work in progress.

## Building

Configure it with:

```
mkdir build
cd build
../configure --enable-sdl --disable-cocoa --target-list=arm-softmmu --disable-capstone --disable-pie --disable-slirp --disable-werror --extra-cflags="-g" --extra-ldflags='-lcrypto'
make -j12
```

## How to get the files required to run the emulator:

The emulator requires files that I probably shouldn't share. Luckily, if you have an iPod Nano 3G, you can dump and decrypt these files yourself. More complete instructions will be added later. For now, these instructions are incomplete.

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
reset && make -j12 && ./arm-softmmu/qemu-system-arm -s -S -M iPod-Nano3G,bootrom=/home/tucker/Development/qemu-ipod-nano/build/s5l8702-bootrom.bin,iboot=/home/tucker/Development/qemu-ipod-nano/build/iboot_204_n45ap.bin,nand=/home/tucker/Development/qemu-ipod-nano/build/nand -serial mon:stdio -cpu max -m 1G -d unimp
```
