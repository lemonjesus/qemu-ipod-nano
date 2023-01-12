#!/bin/bash

# This script is used to build the NOR flash image for the emulator from dumps from the iPod

# Usage: ./build_nor.sh <full nor dump> <decrypted efi> <output.bin>

# step 0: copy the full nor dump to the destination
cp $1 $3

# step 1: patch bytes at 0x8000 - this is the IM3 header for the efi bootloader, but I don't remember why these bytes are important. I think only the signature changes, but the bootrom skips this check, so... I dunno.
echo "ODcwMjEuMAEAAAAAAPgBAJ9gIH4eKfJNUWHFtupCLKsAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAANla1kVUfIADi+ZVmLYId60=" | base64 -d | dd of=$3 bs=1 seek=32768 conv=notrunc

# step 2: put the decrypted efi at 0x8800
dd if=$2 of=$3 bs=1 seek=34816 conv=notrunc

# step 3: sha1 the output and compare it to a known good hash
shasum1=$(sha1sum $3)

if [ "$shasum1" = "91f788ddf058d1946bb70c056d7ebebf2929b403  $3" ]; then
    echo "$3 rendered correctly!"
    exit 0
else
    echo "The hash of $3 is incorrect! Make sure you're supplying the right files."
    exit 1
fi