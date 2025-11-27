// MS-DOS boot sector: loads IO.SYS from the root directory and executes it.

#ifndef __GNUC__
#error "This code requires GCC with inline assembly support"
#endif

asm(".code16gcc");

// Boot sector layout: 3-byte JMP, 59-byte BPB, then code.
asm("jmp _start");
asm(".space 0x003b");

typedef char int8_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

// FAT12/16 boot sector structure matching the on-disk layout.
typedef struct __attribute__((packed)) {
  int8_t _a[3];
  int8_t name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fats;
  uint16_t root_entries;
  uint16_t total_sectors;
  uint8_t media_descriptor;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t total_sectors2;
  uint8_t drive_index;
  uint8_t _b;
  uint8_t signature;
  uint32_t id;
  int8_t label[11];
  int8_t type[8];
  uint8_t _c[448];
  uint16_t sig;
} boot_t;

// Track current disk operation: LBA address and sectors per track (from BIOS).
typedef struct {
  uint8_t sectors;
  uint32_t lba;
} disk_t;

// FAT directory entry: 8.3 filename format plus metadata.
typedef struct __attribute__((packed)) {
  int8_t filename[8];
  int8_t extension[3];
  uint8_t attributes;
  uint8_t _a;
  uint8_t create_time_us;
  uint16_t create_time;
  uint16_t create_date;
  uint16_t last_access_date;
  uint8_t _b[2];
  uint16_t last_modified_time;
  uint16_t last_modified_date;
  uint16_t cluster;
  uint32_t size;
} entry_t;

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
int8_t const *io_sys_name = "IO      SYS";

// Multi-purpose buffer: first for root directory, then for IO.SYS itself.
uint8_t *buffer;

// Number of sectors to read in the next disk operation.
uint8_t sector_count;

// Current directory entry being examined during search.
entry_t const *current_entry;

// Compare directory entry name against "IO      SYS" (8.3 filename format).
// Standard string comparison: checks until mismatch, null terminator, or end of
// string.
int8_t is_io_sys(void) {
  uint16_t i;
  for (i = 0; i < FILENAME_EXT_LEN - 1 && ((int8_t *)current_entry)[i] &&
              ((int8_t *)current_entry)[i] == io_sys_name[i];
       ++i)
    ;
  return ((int8_t *)current_entry)[i] - io_sys_name[i];
}

// Read sectors from disk using BIOS INT 13h, AH=02h.
// Converts LBA to CHS since older BIOS doesn't support LBA addressing.
void read_sectors(void) {
  uint32_t sectors_per_cylinder = boot_sector->heads * disk_info->sectors;
  uint16_t cylinder = disk_info->lba / sectors_per_cylinder;
  uint16_t head = (disk_info->lba % sectors_per_cylinder) / disk_info->sectors;

  // Pack cylinder (10 bits) and sector (6 bits) into CX register format.
  cylinder <<= 8;
  cylinder |=
      ((disk_info->lba % sectors_per_cylinder) % disk_info->sectors) + 1;

  // INT 13h, AH=02h: Read sectors into memory
  // AL=sector_count, ES:BX=buffer, CX=cylinder/sector, DH=head, DL=drive
  asm("int $0x13"
      :
      : "a"(0x0200 | sector_count), "b"(buffer), "c"(cylinder),
        "d"((head << 8) | FIRST_HDD));
}

uint16_t _start(void) {
  // Get disk geometry using INT 13h, AH=08h.
  // CL bits 0-5 contain sectors per track.
  asm("int $0x13"
      : "=c"(disk_info->sectors)
      : "a"(0x0800), "d"(FIRST_HDD)
      : "bx");
  disk_info->sectors &= 0b00111111;

  // Calculate root directory location: after reserved sectors and both FATs.
  buffer = (uint8_t *)ROOT_DIR_ADDR;
  disk_info->lba = boot_sector->reserved_sectors +
                   (boot_sector->fats * boot_sector->sectors_per_fat);
  sector_count = boot_sector->root_entries * sizeof(entry_t) /
                 boot_sector->bytes_per_sector;
  read_sectors();

  // Scan root directory entries for IO.SYS.
  for (current_entry = (entry_t *)buffer;; ++current_entry)
    if (is_io_sys() == 0) {
      // Found IO.SYS. Calculate its location in the data area.
      // FAT cluster numbering starts at 2, so offset = (cluster - 2) *
      // sectors_per_cluster.
      buffer = (uint8_t *)IO_SYS_LOAD_ADDR;
      disk_info->lba += sector_count + (current_entry->cluster - 2) *
                                           boot_sector->sectors_per_cluster;
      sector_count = IO_SYS_SECTORS;
      read_sectors();

      // Transfer control to IO.SYS at 0000:0700
      asm("jmpw %0, %1" : : "g"(0x0000), "g"(IO_SYS_LOAD_ADDR));
    }

  return 0;
}
