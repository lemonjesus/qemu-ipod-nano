# GDB may have ./.gdbinit loading disabled by default.  In that case you can
# follow the instructions it prints.  They boil down to adding the following to
# your home directory's ~/.gdbinit file:
#
#   add-auto-load-safe-path /path/to/qemu/.gdbinit

# Load QEMU-specific sub-commands and settings
source scripts/qemu-gdb.py

set pagination off
target remote localhost:1234

file /home/tucker/Development/qemu-ipod-nano/build/bootrom_symbols
file /home/tucker/Development/qemu-ipod-nano/build/efi_adjusted_symbols

tui new-layout vmdebug {-horizontal asm 1 regs 1} 2 cmd 1
layout vmdebug
focus cmd
set confirm off

define sf
  until *($pc + 6)
end

define dump
  x/32bx $pc
end

# break on a CPU exception
break *0x10
break *0x9f022c0

# first instruction of a module! maps to 40080242 efi_DxeMain_entrypoint in Ghidra, offset of 0x360C1000
# break *0x9fbf242

skip function efi_DxeMain_CoreLocateProtocol ()
break *0x9ef802e

# awatch *0x89ea5020

continue
