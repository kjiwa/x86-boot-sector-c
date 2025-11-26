CFLAGS=-Wall

all:

clean:
	rm -rf *~ *.o *.elf boot io.sys c.img

boot.o: boot.c
	$(CC) $(CFLAGS) -Os -m32 -fno-pic -c -o $@ $^

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
	mkfs.vfat c.img

deploy: disk boot io
	dd if=boot of=c.img bs=1 count=3 conv=notrunc
	dd if=boot of=c.img skip=36 seek=36 bs=1 count=1 conv=notrunc
	dd if=boot of=c.img skip=62 seek=62 bs=1 count=450 conv=notrunc
	mcopy -i c.img io.sys ::io.sys
