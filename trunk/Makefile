CFLAGS=-Wall

all:

clean:
	rm -rf *~ *.o *.elf boot io.sys c.img

boot.o: boot.c
	$(CC) $(CFLAGS) -Os -m32 -c -o $@ $^

io.o: io.c
	$(CC) $(CFLAGS) -O -fno-builtin -m32 -c -o $@ $^

boot: boot.o
	$(LD) $(LDFLAGS) -T boot.ld -melf_i386 -o $@.elf $^
	objcopy -O binary $@.elf $@

io: io.o
	$(LD) $(LDFLAGS) -T io.ld -melf_i386 -o $@.elf $^
	objcopy -O binary $@.elf $@.sys

disk:
	dd if=/dev/zero of=c.img bs=512 count=20160
	sudo losetup /dev/loop0 c.img
	sudo mkfs.vfat /dev/loop0
	sudo losetup -d /dev/loop0

deploy: disk boot io
	dd if=boot of=c.img bs=1 count=3 conv=notrunc
	dd if=boot of=c.img skip=36 seek=36 bs=1 count=1 conv=notrunc
	dd if=boot of=c.img skip=62 seek=62 bs=1 count=450 conv=notrunc
	sudo losetup /dev/loop0 c.img
	sudo mount -t vfat /dev/loop0 /mnt/c.img
	sudo cp io.sys /mnt/c.img/io.sys
	sudo umount /mnt/c.img
	sudo losetup -d /dev/loop0
