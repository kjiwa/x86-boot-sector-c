.POSIX:
.SUFFIXES:
.SUFFIXES: .c .o .elf

# Tools
CC = gcc
LD = ld
OBJCOPY = objcopy
DD = dd
MKFS = mkfs.vfat
MCOPY = mcopy

# Flags
CFLAGS = -Wall -Wextra -m32 -fno-pic -ffreestanding -nostdlib
LDFLAGS = -melf_i386 -nostdlib
BOOT_CFLAGS = $(CFLAGS) -Os
IO_CFLAGS = $(CFLAGS) -O2 -fno-builtin

# Targets
DISK_IMAGE = c.img
BOOT_SECTOR = boot
IO_SYSTEM = io.sys
DISK_SIZE = 20160

# Default target
all: $(DISK_IMAGE)

# Build boot sector
boot.o: boot.c boot.ld
	$(CC) $(BOOT_CFLAGS) -c -o $@ boot.c

$(BOOT_SECTOR): boot.o boot.ld
	$(LD) $(LDFLAGS) -T boot.ld -o $@.elf $<
	$(OBJCOPY) -O binary $@.elf $@

# Build IO.SYS
io.o: io.c io.ld
	$(CC) $(IO_CFLAGS) -c -o $@ io.c

$(IO_SYSTEM): io.o io.ld
	$(LD) $(LDFLAGS) -T io.ld -o io.elf $<
	$(OBJCOPY) -O binary io.elf $@

# Create disk image
$(DISK_IMAGE): $(BOOT_SECTOR) $(IO_SYSTEM)
	$(DD) if=/dev/zero of=$@ bs=512 count=$(DISK_SIZE) status=none
	$(MKFS) -F 12 $@ >/dev/null 2>&1
	$(DD) if=$(BOOT_SECTOR) of=$@ bs=1 count=3 conv=notrunc status=none
	printf '\x80' | $(DD) of=$@ seek=36 bs=1 count=1 conv=notrunc status=none
	$(DD) if=$(BOOT_SECTOR) of=$@ skip=62 seek=62 bs=1 count=450 conv=notrunc status=none
	MTOOLS_SKIP_CHECK=1 $(MCOPY) -i $@ $(IO_SYSTEM) ::$(IO_SYSTEM)

# Run in Bochs
run: $(DISK_IMAGE)
	bochs -q -f bochsrc

# Cleanup
clean:
	rm -f *.o *.elf $(BOOT_SECTOR) $(IO_SYSTEM) $(DISK_IMAGE) bochsout.txt

.PHONY: all run clean