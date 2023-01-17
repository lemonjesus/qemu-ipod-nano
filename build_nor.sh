#!/bin/bash

# This script is used to build the NOR flash image for the emulator from dumps from the iPod

# Usage: ./build_nor.sh <full nor dump> <decrypted efi> <dir with jpegs> <output.bin>

# step 0: copy the full nor dump to the destination
cp $1 $4

# step 1: patch bytes at 0x8000 - this is the IM3 header for the efi bootloader, but I don't remember why these bytes are important. I think only the signature changes, but the bootrom skips this check, so... I dunno.
echo "ODcwMjEuMAEAAAAAAPgBAJ9gIH4eKfJNUWHFtupCLKsAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANla1kVUfIADi+ZVmLYId60=" | base64 -d | dd of=$4 bs=1 seek=$((0x8000)) conv=notrunc

# step 2: put the decrypted efi at 0x8800
dd if=$2 of=$4 bs=1 seek=$((0x8800)) conv=notrunc

# step 3: fix the JPEGs so they're not encrypted any more

# step 3a: write the JPEGs to the NOR flash image
dd if=$3/0x000930E0.chrgflsh.jpg of=$4 bs=1 seek=$((0x930E0 + 0x200)) conv=notrunc
dd if=$3/0x000954E0.bdhwflsh.jpg of=$4 bs=1 seek=$((0x954E0 + 0x200)) conv=notrunc
dd if=$3/0x000978E0.bdswflsh.jpg of=$4 bs=1 seek=$((0x978E0 + 0x200)) conv=notrunc
dd if=$3/0x0009AB30.lbatflsh.jpg of=$4 bs=1 seek=$((0x9AB30 + 0x200)) conv=notrunc
dd if=$3/0x0009CCC0.applflsh.jpg of=$4 bs=1 seek=$((0x9CCC0 + 0x200)) conv=notrunc

# step 3b: write the sha1sums to the headers
sha1sum 0x000930E0.chrgflsh.jpg | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x930E0 + 0x1C)) conv=notrunc
sha1sum 0x000954E0.bdhwflsh.jpg | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x954E0 + 0x1C)) conv=notrunc
sha1sum 0x000978E0.bdswflsh.jpg | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x978E0 + 0x1C)) conv=notrunc
sha1sum 0x0009AB30.lbatflsh.jpg | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x9AB30 + 0x1C)) conv=notrunc
sha1sum 0x0009CCC0.applflsh.jpg | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x9CCC0 + 0x1C)) conv=notrunc

# step 3c: change the AES Key ID to 0x01 (the GID key which the emulator ignores)
echo -n -e "\x01" | dd of=$4 bs=1 seek=$((0x930E0 + 0x8)) conv=notrunc
echo -n -e "\x01" | dd of=$4 bs=1 seek=$((0x954E0 + 0x8)) conv=notrunc
echo -n -e "\x01" | dd of=$4 bs=1 seek=$((0x978E0 + 0x8)) conv=notrunc
echo -n -e "\x01" | dd of=$4 bs=1 seek=$((0x9AB30 + 0x8)) conv=notrunc
echo -n -e "\x01" | dd of=$4 bs=1 seek=$((0x9CCC0 + 0x8)) conv=notrunc

# step 3d: clear the header checksums
dd if=/dev/zero of=$4 bs=1 count=20 seek=$((0x930E0 + 0x1d4)) conv=notrunc
dd if=/dev/zero of=$4 bs=1 count=20 seek=$((0x954E0 + 0x1d4)) conv=notrunc
dd if=/dev/zero of=$4 bs=1 count=20 seek=$((0x978E0 + 0x1d4)) conv=notrunc
dd if=/dev/zero of=$4 bs=1 count=20 seek=$((0x9AB30 + 0x1d4)) conv=notrunc
dd if=/dev/zero of=$4 bs=1 count=20 seek=$((0x9CCC0 + 0x1d4)) conv=notrunc

# step 3e: calculate and write the header checksums
dd if=$4 bs=1 count=$((0x200)) skip=$((0x930E0)) | sha1sum | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x930E0 + 0x1d4)) conv=notrunc
dd if=$4 bs=1 count=$((0x200)) skip=$((0x954E0)) | sha1sum | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x954E0 + 0x1d4)) conv=notrunc
dd if=$4 bs=1 count=$((0x200)) skip=$((0x978E0)) | sha1sum | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x978E0 + 0x1d4)) conv=notrunc
dd if=$4 bs=1 count=$((0x200)) skip=$((0x9AB30)) | sha1sum | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x9AB30 + 0x1d4)) conv=notrunc
dd if=$4 bs=1 count=$((0x200)) skip=$((0x9CCC0)) | sha1sum | cut -c -40 | xxd -r -p | dd of=$4 bs=1 seek=$((0x9CCC0 + 0x1d4)) conv=notrunc
