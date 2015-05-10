/**
 * An MS-DOS boot sector that reads the hard disk drive's root directory,
 * searches for IO.SYS, loads the first 3 sectors into memory, and executes it.
 */

// Tell GCC to emit 16-bit opcodes.
asm (".code16gcc");

// The first 3 bytes are reserved for a JMP instruction that we use to jump to
// the _start() method. The next 59 bytes are reserved for the OEM name and the
// BIOS parameter block. We set drive_index to be 0x80 to indicate we are
// booting from a hard disk drive.
asm ("jmp _start");
asm (".space 0x0021");
asm (".byte 0x80");
asm (".space 0x001c");

/**
 * Standard type definitions for the x86 in real mode.
 */
typedef char           s8_t;
typedef unsigned char  u8_t;
typedef unsigned short u16_t;
typedef unsigned long  u32_t;

/**
 * The boot sector.
 * @param _a                  3 reserved bytes used for a JMP instruction.
 * @param name                The OEM name of the volume.
 * @param bytes_per_sector    The number of bytes per sector.
 * @param sectors_per_cluster The number of sectors per cluster.
 * @param reserved_sectors    The number of reserved sectors.
 * @param fats                The number of file allocation tables.
 * @param root_entries        The number of entries in the root directory.
 * @param total_sectors       The number of hard disk sectors. If zero, use the
 *                                value in total_sectors2.
 * @param media_descriptor    The media descriptor.
 * @param sectors_per_fat     The number of sectors per file allocation table.
 * @param sectors_per_track   The number of sectors per track.
 * @param heads               The number of hard disk heads.
 * @param hidden_sectors      The number of hidden sectors.
 * @param total_sectors2      The number of hard disk sectors.
 * @param drive_index         The drive index.
 * @param _b                  Reserved.
 * @param signature           The extended boot signature.
 * @param id                  The volume ID.
 * @param label               The partition volume label.
 * @param type                The file system type.
 * @param _c                  Code to be executed.
 * @param sig                 The boot signature. Always 0xaa55.
 */
typedef struct
{
  s8_t  _a[3];
  s8_t  name[8];
  u16_t bytes_per_sector;
  u8_t  sectors_per_cluster;
  u16_t reserved_sectors;
  u8_t  fats;
  u16_t root_entries;
  u16_t total_sectors;
  u8_t  media_descriptor;
  u16_t sectors_per_fat;
  u16_t sectors_per_track;
  u16_t heads;
  u32_t hidden_sectors;
  u32_t total_sectors2;
  u8_t  drive_index;
  u8_t  _b;
  u8_t  signature;
  u32_t id;
  s8_t  label[11];
  s8_t  type[8];
  u8_t  _c[448];
  u16_t sig;
} __attribute__ ((packed)) boot_t;

/**
 * Hard disk drive information.
 * @param sectors The number of sectors on the disk.
 * @param lba     The LBA of the block to read from the hard disk drive.
 */
typedef struct
{
  u8_t  sectors;
  u32_t lba;
} FILE;

/**
 * A root directory entry.
 * @param filename           The file name.
 * @param extension          The file extension.
 * @param attributes         File attributes.
 * @param _a                 Reserved.
 * @param create_time_us     The microsecond value of the creation time.
 * @param create_time        The creation time.
 * @param create_date        The creation date.
 * @param last_access_date   The date the file was last accessed.
 * @param _b                 Reserved.
 * @param last_modified_time The time the file was last modified.
 * @param last_modified_date The date the file was last modified.
 * @param cluster            The cluster containing the start of the file.
 * @param size               The file size in bytes.
 */
typedef struct
{
  s8_t  filename[8];
  s8_t  extension[3];
  u8_t  attributes;
  u8_t  _a;
  u8_t  create_time_us;
  u16_t create_time;
  u16_t create_date;
  u16_t last_access_date;
  u8_t  _b[2];
  u16_t last_modified_time;
  u16_t last_modified_date;
  u16_t cluster;
  u32_t size;
} __attribute__ ((packed)) entry_t;

/**
 * A pointer to the boot sector. After the BIOS POST, the boot sector is loaded
 * into memory at 0x7c00.
 */
boot_t const* _bs = (boot_t*) 0x7c00;

/**
 * A pointer to the hard disk drive information. We use 0x7e00, the first block
 * of available memory next to the boot sector.
 */
FILE* _disk = (FILE*) 0x7e00;

/**
 * A constant containing the name of the binary used to initialize the system
 * and device drivers. This is typically IO.SYS or IBMBIO.COM.
 */
s8_t const* _io_bin = "IO      SYS";

/**
 * A pointer to a general-purpose buffer in memory. Used to store the root
 * directory entries and the first 3 sectors of IO.SYS.
 */
u8_t* _buffer;

/**
 * A variable used by the read() method to indicate how many sectors to read
 * from the hard disk into memory.
 */
u8_t _size;

/**
 * A pointer to the root directory entry currently being read.
 */
entry_t const* _entry;

/**
 * Checks if the current root directory entry has the name IO.SYS by performing
 * a string comparison.
 * @return 0 if the current root directory entry has the name IO.SYS, a
 *     negative value if the entry name is less than IO.SYS, or a positive
 *     value if the entry name is greater than IO.SYS.
 */
s8_t
iosyscmp()
{
  u16_t i;
  for (i = 0;
       i < 10 && ((s8_t*) _entry)[i] && ((s8_t*) _entry)[i] == _io_bin[i];
       ++i);
  return ((s8_t*) _entry)[i] - _io_bin[i];
}

/**
 * Reads sectors at the provided LBA from the hard disk into memory. Uses the
 * value stored in _size as the number of sectors to read.
 * @return 0
 */
u16_t
read()
{
  // convert the LBA into CHS values
  u32_t t = _bs->heads * _disk->sectors;
  u16_t c = _disk->lba / t;
  u16_t h = (_disk->lba % t) / _disk->sectors;
  c <<= 8;
  c |= ((_disk->lba % t) % _disk->sectors) + 1;

  // read sectors from the hard disk into memory at 0x0500
  asm ("int $0x13" : : "a"(0x0200 | _size), "b"(_buffer), "c"(c), "d"((h << 8) | 0x0080));
  return 0;
}

/**
 * Reads drive information, searches for IO.SYS, loads the first 3 sectors into
 * memory, and begins executing it.
 * @return 0
 */
u16_t
_start()
{
  // read hard disk drive information into memory at 0x7e00
  asm ("int $0x13" : "=c"(_disk->sectors) : "a"(0x0800), "d"(0x80) : "bx");
  _disk->sectors &= 0b00111111;

  // read the root directory into memory at 0x500
  _buffer = (u8_t*) 0x0500;
  _disk->lba = _bs->reserved_sectors + (_bs->fats * _bs->sectors_per_fat);
  _size = _bs->root_entries * sizeof(entry_t) / _bs->bytes_per_sector;
  read();

  // iterate over root directory entries and look for IO.SYS
  for (_entry = (entry_t*) _buffer; ; ++_entry)
    if (iosyscmp() == 0) {
      // load the first 3 sectors of IO.SYS into memory at 0x0700
      _buffer = (u8_t*) 0x0700;
      _disk->lba += _size + (_entry->cluster - 2) * _bs->sectors_per_cluster;
      _size = 3;
      read();

      // execute IO.SYS
      asm ("jmpw %0, %1" : : "g"(0x0000), "g"(0x0700));
    }

  return 0;
}
