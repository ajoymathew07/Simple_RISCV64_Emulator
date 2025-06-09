/* custom.ld */
SECTIONS
{
  . = 0x80000000;

  .text : {
    *(.text)
  }

  .rodata : {
    *(.rodata)
  }

  .data : {
    *(.data)
  }

  .sdata : {
    _gp = 0x8000102c;  /* Set gp to constant */
    *(.sdata)
  }

  .bss : {
    *(.bss)
  }

  /DISCARD/ : { *(.eh_frame) *(.comment) }
}
