CFLAGS=-Wall

all:

clean:
	rm -rf *~ *.o *.elf boot io.com

boot.o: boot.c
	$(CC) $(CFLAGS) -Os -fno-builtin-strncmp -c -o $@ $^

io.com.o: io.com.c
	$(CC) $(CFLAGS) -O -fno-builtin-puts -fno-builtin-putchar -c -o $@ $^

boot: boot.o
	$(LD) $(LDFLAGS) -T boot.ld -o $@.elf $^
	objcopy -O binary $@.elf $@

io.com: io.com.o
	$(LD) $(LDFLAGS) -T io.com.ld -o $@.elf $^
	objcopy -O binary $@.elf $@

disk:
	dd if=/dev/zero of=c.img bs=512 count=20160
	sudo losetup /dev/loop0 c.img
	sudo mkfs.vfat /dev/loop0
	sudo losetup -d /dev/loop0

deploy: boot io.com
	dd if=boot of=c.img bs=1 count=3 conv=notrunc
	dd if=boot of=c.img skip=36 seek=36 bs=1 count=1 conv=notrunc
	dd if=boot of=c.img skip=62 seek=62 bs=1 count=450 conv=notrunc
	sudo losetup /dev/loop0 c.img
	sudo mount -t vfat /dev/loop0 /mnt/c.img
	sudo cp io.com /mnt/c.img/io.com
	sudo umount /mnt/c.img
	sudo losetup -d /dev/loop0
