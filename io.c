asm (".code16gcc");
asm ("jmp start");

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
start()
{
  puts("IO.SYS\r\n");
  while (1);
}
