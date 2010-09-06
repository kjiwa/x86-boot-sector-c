#define _DRIVE_INDEX 0x80

asm (".code16gcc");

asm ("jmp start");
asm (".space 0x0021");
asm (".byte 0x80");
asm (".space 0x001c");

typedef char           int8_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;

typedef struct
{
	int8_t   _a[3];
	int8_t   name[8];
	uint16_t bytes_per_sector;
	uint8_t  sectors_per_cluster;
	uint16_t reserved_sectors;
	uint8_t  fats;
	uint16_t root_entries;
	uint16_t total_sectors;
	uint8_t  media_descriptor;
	uint16_t sectors_per_fat;
	uint16_t spt;
	uint16_t heads;
	uint32_t hidden_sectors;
	uint32_t total_sectors2;
	uint8_t  drive_index;
	uint8_t  _b;
	uint8_t  signature;
	uint32_t id;
	int8_t   label[11];
	int8_t   type[8];
	int8_t   _c[448];
	uint16_t sig;
} __attribute__ ((packed)) boot_t;

typedef struct
{
	uint32_t lba;
	uint8_t  heads;
	uint8_t  sectors;
} FILE;

typedef struct
{
	int8_t   filename[8];
	int8_t   extension[3];
	uint8_t  attributes;
	uint8_t  _a;
	uint8_t  create_time_us;
	uint16_t create_time;
	uint16_t create_date;
	uint16_t last_access_date;
	uint8_t  _b[2];
	uint16_t last_modified_time;
	uint16_t last_modified_date;
	uint16_t cluster;
	uint32_t size;
} __attribute__ ((packed)) entry_t;

boot_t* _bs = (boot_t*) 0x7c00;
FILE* _disk = (FILE*) 0x7e00;
uint8_t* _buf = (uint8_t*) 0x0500;

int8_t
strncmp(const int8_t* str1, const int8_t* str2, uint16_t count)
{
	uint16_t i;
	for (i = 0; i < count - 1 && str1[i] && str2[i] && str1[i] == str2[i]; ++i);
	return str1[i] - str2[i];
}

void
read(uint8_t* buffer, uint32_t lba, uint8_t size)
{
	uint32_t t = _disk->heads * _disk->sectors;
	uint16_t c = lba / t;
	uint16_t h = (lba % t) / _disk->sectors;
	c <<= 8;
	c |= ((lba % t) % _disk->sectors) + 1;
	asm ("int $0x0013" : : "a"(0x0200 | size), "c"(c), "d"((h << 8) | _DRIVE_INDEX), "b"(buffer));
}

void
start()
{
	entry_t* entry;
	uint16_t cx, dx;
	uint8_t size;
	uint32_t lba;

	size = _bs->root_entries * sizeof(entry_t) / _bs->bytes_per_sector;
	lba = _bs->reserved_sectors + (_bs->fats * _bs->sectors_per_fat);

	asm ("int $0x0013" : "=c"(cx), "=d"(dx) : "a"(0x0800), "d"(_DRIVE_INDEX));
	_disk->sectors = cx & 0x003f;
	_disk->heads = dx >> 8;

	read(_buf, lba, size);

	for (entry = (entry_t*) _buf; ; ++entry)
		if (strncmp(entry->filename, "IO ", 3) == 0) {
			lba += size + (entry->cluster - 2) * _bs->sectors_per_cluster;
			read(_buf, lba, 3);
			asm ("jmp 0x0500");
		}
}
