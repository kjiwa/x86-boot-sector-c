// MS-DOS boot sector: loads IO.SYS from the root directory and executes it.

asm(".code16gcc");

// Boot sector layout: 3-byte JMP, 59-byte BPB, then code.
asm("jmp _start");
asm(".space 0x003b");

typedef char s8_t;
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;

// FAT12/16 boot sector structure matching the on-disk layout.
typedef struct {
  s8_t _a[3];
  s8_t name[8];
  u16_t bytes_per_sector;
  u8_t sectors_per_cluster;
  u16_t reserved_sectors;
  u8_t fats;
  u16_t root_entries;
  u16_t total_sectors;
  u8_t media_descriptor;
  u16_t sectors_per_fat;
  u16_t sectors_per_track;
  u16_t heads;
  u32_t hidden_sectors;
  u32_t total_sectors2;
  u8_t drive_index;
  u8_t _b;
  u8_t signature;
  u32_t id;
  s8_t label[11];
  s8_t type[8];
  u8_t _c[448];
  u16_t sig;
} __attribute__((packed)) boot_t;

// Track current disk operation: LBA address and sectors per track (from BIOS).
typedef struct {
  u8_t sectors;
  u32_t lba;
} disk_t;

// FAT directory entry: 8.3 filename format plus metadata.
typedef struct {
  s8_t filename[8];
  s8_t extension[3];
  u8_t attributes;
  u8_t _a;
  u8_t create_time_us;
  u16_t create_time;
  u16_t create_date;
  u16_t last_access_date;
  u8_t _b[2];
  u16_t last_modified_time;
  u16_t last_modified_date;
  u16_t cluster;
  u32_t size;
} __attribute__((packed)) entry_t;

#define BOOT_SECTOR_ADDR 0x7c00
#define DISK_INFO_ADDR 0x7e00
#define ROOT_DIR_ADDR 0x0500
#define IO_SYS_LOAD_ADDR 0x0700
#define FIRST_HDD 0x80
#define IO_SYS_SECTORS 3
#define FILENAME_EXT_LEN 11

// BIOS loads the boot sector here during POST.
boot_t const *boot_sector = (boot_t *)BOOT_SECTOR_ADDR;

// Store disk geometry info right after boot sector in free memory.
disk_t *disk_info = (disk_t *)DISK_INFO_ADDR;

// IO.SYS in 8.3 format: "IO      " + "SYS"
s8_t const *io_sys_name = "IO      SYS";

// Multi-purpose buffer: first for root directory, then for IO.SYS itself.
u8_t *buffer;

// Number of sectors to read in the next disk operation.
u8_t sector_count;

// Current directory entry being examined during search.
entry_t const *current_entry;

// Compare current directory entry name against "IO      SYS".
// Checks first 10 chars in loop, then 11th char in return (not a bug).
s8_t is_io_sys(void) {
  u16_t i;
  for (i = 0; i < FILENAME_EXT_LEN - 1 && ((s8_t *)current_entry)[i] &&
              ((s8_t *)current_entry)[i] == io_sys_name[i];
       ++i)
    ;
  return ((s8_t *)current_entry)[i] - io_sys_name[i];
}

// Read sectors from disk using BIOS INT 13h, AH=02h.
// Converts LBA to CHS since older BIOS doesn't support LBA mode.
void read_sectors(void) {
  u32_t sectors_per_cylinder = boot_sector->heads * disk_info->sectors;
  u16_t cylinder = disk_info->lba / sectors_per_cylinder;
  u16_t head = (disk_info->lba % sectors_per_cylinder) / disk_info->sectors;

  // Pack cylinder and sector into CX register format.
  cylinder <<= 8;
  cylinder |=
      ((disk_info->lba % sectors_per_cylinder) % disk_info->sectors) + 1;

  // AH=02h (read), AL=sector_count, ES:BX=buffer, CX=cyl/sec, DH=head, DL=drive
  asm("int $0x13"
      :
      : "a"(0x0200 | sector_count), "b"(buffer), "c"(cylinder),
        "d"((head << 8) | FIRST_HDD));
}

u16_t _start(void) {
  // Get disk geometry using BIOS INT 13h, AH=08h.
  // Returns sectors per track in CL (bits 0-5).
  asm("int $0x13"
      : "=c"(disk_info->sectors)
      : "a"(0x0800), "d"(FIRST_HDD)
      : "bx");
  disk_info->sectors &= 0b00111111;

  // Calculate root directory location: after reserved sectors and FATs.
  buffer = (u8_t *)ROOT_DIR_ADDR;
  disk_info->lba = boot_sector->reserved_sectors +
                   (boot_sector->fats * boot_sector->sectors_per_fat);
  sector_count = boot_sector->root_entries * sizeof(entry_t) /
                 boot_sector->bytes_per_sector;
  read_sectors();

  // Scan root directory for IO.SYS.
  for (current_entry = (entry_t *)buffer;; ++current_entry)
    if (is_io_sys() == 0) {
      // Found IO.SYS. Calculate its location in data area.
      // Clusters start at 2, so subtract 2 to get offset from data area start.
      buffer = (u8_t *)IO_SYS_LOAD_ADDR;
      disk_info->lba += sector_count + (current_entry->cluster - 2) *
                                           boot_sector->sectors_per_cluster;
      sector_count = IO_SYS_SECTORS;
      read_sectors();

      // Jump to IO.SYS at 0000:0700
      asm("jmpw %0, %1" : : "g"(0x0000), "g"(IO_SYS_LOAD_ADDR));
    }

  return 0;
}
