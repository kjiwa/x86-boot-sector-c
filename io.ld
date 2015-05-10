SECTIONS
{
  . = 0x0700;

  .cseg :
  {
    *(.text);
  }

  .dseg :
  {
    *(.rodata);
  }

  /DISCARD/ :
  {
    *(.eh_frame);
  }
}
