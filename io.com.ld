SECTIONS
{
	. = 0x0500;

	.cseg :
	{
		*(.text);
	}

	.dseg :
	{
		*(.rodata);
	}
}
