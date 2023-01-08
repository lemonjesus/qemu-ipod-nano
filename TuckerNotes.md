Run it with:
```
./arm-softmmu/qemu-system-arm -M iPod-Touch,bootrom=/home/tucker/Development/qemu-ipod-nano/build/s5l8702-bootrom.bin,iboot=/home/tucker/Development/qemu-ipod-nano/build/iboot_204_n45ap.bin,nand=/home/tucker/Development/qemu-ipod-nano/build/nand -serial mon:stdio -cpu max -m 1G -d unimp -pflash /home/tucker/Development/qemu-ipod-nano/build/nor_n45ap.bin
```

Run the iPhone code with:
```
./arm-softmmu/qemu-system-arm -M iPod-Touch,bootrom=/home/tucker/Development/qemu-ipod-nano/build/bootrom_s5l8900,iboot=/home/tucker/Development/qemu-ipod-nano/build/iboot_204_n45ap.bin,nand=/home/tucker/Development/qemu-ipod-nano/build/nand -serial mon:stdio -cpu max -m 1G -d unimp -pflash /home/tucker/Development/qemu-ipod-nano/build/nor_n45ap.bin
```

Configure for finding strange malloc bugs:

```
../configure --enable-sdl --disable-cocoa --target-list=arm-softmmu --disable-capstone --disable-pie --disable-slirp --disable-werror --extra-cflags="-fno-omit-frame-pointer -fsanitize=address -g" --extra-ldflags='-lcrypto -fsanitize=address'
```
