# DOS Boot Sector

This is an x86 boot sector written in C. It is meant to bootstrap a DOS system on a FAT volume. A more detailed explanation of the code is available at https://crimsonglow.ca/~kjiwa/2010/09/dos-boot-sector.html.

[View](boot.c) the source. It works like this at a high level:

* get the drive parameters
* load the contents of the root directory into memory
* look for IO.SYS in the root directory
* load the first 3 sectors of IO.SYS into memory and begin executing it

I've successfully compiled and tested it with the following tools:

* gcc 12.2.0
* binutils 2.40
* dosfstools 4.2
* bochs 2.7

The gcc and binutils host and target are both i386-pc-elf.

```shell
$ make boot    # compile and link the boot sector
$ make io.sys  # compile and link IO.SYS
$ make c.img   # create a 10 MB disk image with a FAT volume, boot sector, and IO.SYS
$ make run     # runs Bochs
```

##### External Links #####

* [SYS.COM Requirements in MS-DOS versions 2.0-6.0](http://support.microsoft.com/en-us/kb/66530/en-us)
* [int 13](http://www.ctyme.com/intr/int-13.htm)
* [Memory Map (x86)](http://wiki.osdev.org/Memory_Map_%28x86%29)
* [File Allocation Table](http://en.wikipedia.org/wiki/File_Allocation_Table)
