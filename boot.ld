SECTIONS
{
  . = 0x7c00;

  .cseg :
  {
    *(.text);
  }

  .dseg :
  {
    *(.rodata);
  }

  .sig : AT(0x7dfe)
  {
    SHORT(0xaa55);
  }

  /DISCARD/ :
  {
    *(.eh_frame);
  }
}
