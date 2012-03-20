asm (".code16gcc");

asm ("jmp start");
asm (".space 0x0021");
asm (".byte 0x80");
asm (".space 0x001c");

typedef char           s8_t;
typedef unsigned char  u8_t;
typedef unsigned short u16_t;
typedef unsigned long  u32_t;

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

typedef struct
{
	u8_t  sectors;
	u32_t lba;
} FILE;

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

const boot_t* const _bs     = (boot_t*) 0x7c00;
FILE*               _disk   = (FILE*)   0x7e00;
const s8_t* const   _io_bin = "IO      SYS";

u8_t*               _buffer;
u8_t                _size;
entry_t*            _entry;

s8_t
iosyscmp()
{
	u8_t i;
	for (i = 0; i < 10 && ((s8_t*) _entry)[i] && ((s8_t*) _entry)[i] == _io_bin[i]; ++i);
	return ((s8_t*) _entry)[i] - _io_bin[i];
}

void
read()
{
	u32_t t = _bs->heads * _disk->sectors;
	u16_t c = _disk->lba / t;
	u16_t h = (_disk->lba % t) / _disk->sectors;
	c <<= 8;
	c |= ((_disk->lba % t) % _disk->sectors) + 1;
	asm ("int $0x13" : : "a"(0x0200 | _size), "c"(c), "d"((h << 8) | 0x80), "b"(_buffer));
}

void
start()
{
	asm ("int $0x13" : "=c"(_disk->sectors) : "a"(0x0800), "d"(0x80));
	_disk->sectors &= 0b00111111;

	_buffer = (u8_t*) 0x0500;
	_disk->lba = _bs->reserved_sectors + (_bs->fats * _bs->sectors_per_fat);
	_size = _bs->root_entries * sizeof(entry_t) / _bs->bytes_per_sector;
	read();

	for (_entry = (entry_t*) _buffer; ; ++_entry)
		if (iosyscmp() == 0) {
			_buffer = (u8_t*) 0x0700;
			_disk->lba += _size + (_entry->cluster - 2) * _bs->sectors_per_cluster;
			_size = 3;
			read();
			asm ("jmpl $0x0070, $0x0000");
		}
}
