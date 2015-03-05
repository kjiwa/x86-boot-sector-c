This is an x86 boot sector written in C. It is meant to bootstrap a DOS system on a FAT volume.

I've successfully compiled and tested it with the following tools:

* gcc 4.4.3
* binutils 2.20.1
* dosfstools 3.0.7
* bochs 2.4.2

The gcc and binutils host and target are both i386-pc-elf.

```shell
$ make boot      # compile and link the boot sector
$ make io        # compile and link IO.SYS
$ make disk      # create a 10 MB disk image with a FAT volume
$ make deploy    # copy the boot sector and IO.SYS to the disk image
```

It works like this:

* get the drive parameters
* load the contents of the root directory into memory
* look for IO.SYS in the root directory
* load the first 3 sectors of IO.SYS into memory and begin executing it

Check [here](http://www.crimsonglow.ca/~kjiwa/x86-dos-boot-sector-in-c.html) for a more in-depth description of what's going on.

[View](boot.c) the source.
