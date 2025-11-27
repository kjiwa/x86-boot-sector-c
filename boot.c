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
} disk_t;

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

#define BOOT_SECTOR_ADDR  0x7c00
#define DISK_INFO_ADDR    0x7e00
#define ROOT_DIR_ADDR     0x0500
#define IO_SYS_LOAD_ADDR  0x0700
#define FIRST_HDD         0x80
#define IO_SYS_SECTORS    3
#define FILENAME_EXT_LEN  11

/**
 * A pointer to the boot sector. After the BIOS POST, the boot sector is loaded
 * into memory at 0x7c00.
 */
boot_t const* boot_sector = (boot_t*) BOOT_SECTOR_ADDR;

/**
 * A pointer to the hard disk drive information. We use 0x7e00, the first block
 * of available memory next to the boot sector.
 */
disk_t* disk_info = (disk_t*) DISK_INFO_ADDR;

/**
 * A constant containing the name of the binary used to initialize the system
 * and device drivers. This is typically IO.SYS or IBMBIO.COM.
 */
s8_t const* io_sys_name = "IO      SYS";

/**
 * A pointer to a general-purpose buffer in memory. Used to store the root
 * directory entries and the first 3 sectors of IO.SYS.
 */
u8_t* buffer;

/**
 * A variable used by the read_sectors() method to indicate how many sectors to
 * read from the hard disk into memory.
 */
u8_t sector_count;

/**
 * A pointer to the root directory entry currently being read.
 */
entry_t const* current_entry;

/**
 * Checks if the current root directory entry has the name IO.SYS by performing
 * a string comparison.
 * @return 0 if the current root directory entry has the name IO.SYS, a
 *     negative value if the entry name is less than IO.SYS, or a positive
 *     value if the entry name is greater than IO.SYS.
 */
s8_t
is_io_sys(void)
{
  u16_t i;
  for (i = 0;
       i < FILENAME_EXT_LEN - 1 && 
       ((s8_t*) current_entry)[i] && 
       ((s8_t*) current_entry)[i] == io_sys_name[i];
       ++i);
  return ((s8_t*) current_entry)[i] - io_sys_name[i];
}

/**
 * Reads sectors at the provided LBA from the hard disk into memory. Uses the
 * value stored in sector_count as the number of sectors to read.
 * @return 0
 */
void
read_sectors(void)
{
  // convert the LBA into CHS values
  u32_t sectors_per_cylinder = boot_sector->heads * disk_info->sectors;
  u16_t cylinder = disk_info->lba / sectors_per_cylinder;
  u16_t head = (disk_info->lba % sectors_per_cylinder) / disk_info->sectors;
  cylinder <<= 8;
  cylinder |= ((disk_info->lba % sectors_per_cylinder) % disk_info->sectors) + 1;

  // read sectors from the hard disk into memory
  asm ("int $0x13" : : "a"(0x0200 | sector_count), "b"(buffer), "c"(cylinder), "d"((head << 8) | FIRST_HDD));
}

/**
 * Reads drive information, searches for IO.SYS, loads the first 3 sectors into
 * memory, and begins executing it.
 * @return 0
 */
u16_t
_start(void)
{
  // read hard disk drive information into memory
  asm ("int $0x13" : "=c"(disk_info->sectors) : "a"(0x0800), "d"(FIRST_HDD) : "bx");
  disk_info->sectors &= 0b00111111;

  // read the root directory into memory
  buffer = (u8_t*) ROOT_DIR_ADDR;
  disk_info->lba = boot_sector->reserved_sectors + (boot_sector->fats * boot_sector->sectors_per_fat);
  sector_count = boot_sector->root_entries * sizeof(entry_t) / boot_sector->bytes_per_sector;
  read_sectors();

  // iterate over root directory entries and look for IO.SYS
  for (current_entry = (entry_t*) buffer; ; ++current_entry)
    if (is_io_sys() == 0) {
      // load the first 3 sectors of IO.SYS into memory
      buffer = (u8_t*) IO_SYS_LOAD_ADDR;
      disk_info->lba += sector_count + (current_entry->cluster - 2) * boot_sector->sectors_per_cluster;
      sector_count = IO_SYS_SECTORS;
      read_sectors();

      // execute IO.SYS
      asm ("jmpw %0, %1" : : "g"(0x0000), "g"(IO_SYS_LOAD_ADDR));
    }

  return 0;
}
