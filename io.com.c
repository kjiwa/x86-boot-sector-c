asm (".code16gcc");
asm ("jmp _start");

void
putchar(char c)
{
	asm ("int $0x0010" : : "a"(0x0e00 | c));
}

void
puts(char* s)
{
	for (; *s; ++s)
		putchar(*s);
}

void
_start()
{
	puts("IO.COM\r\n");
	while (1);
}
