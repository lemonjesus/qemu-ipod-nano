# GDB may have ./.gdbinit loading disabled by default.  In that case you can
# follow the instructions it prints.  They boil down to adding the following to
# your home directory's ~/.gdbinit file:
#
#   add-auto-load-safe-path /path/to/qemu/.gdbinit

# Load QEMU-specific sub-commands and settings
source scripts/qemu-gdb.py

set pagination off
target remote localhost:1234

tui new-layout vmdebug {-horizontal asm 1 regs 1} 2 cmd 1
layout vmdebug
focus cmd
set confirm off

break *0x2200e604
continue
