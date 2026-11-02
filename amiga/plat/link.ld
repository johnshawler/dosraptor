/* Amiga hunk executable: one CODE hunk (text+rodata+data) at 0, one BSS hunk at 0x10000000; converted by elf2hunk.py */
ENTRY(_start)
SECTIONS
{
  . = 0;
  .text   : { *(.text.start) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.* .eh_frame) }
  . = ALIGN(4);
  .data   : { *(.data .data.*) }
  . = ALIGN(4);
  _code_end = .;
  . = 0x10000000;
  .bss    : { *(.bss .bss.* COMMON) }
  . = ALIGN(4);
  _bss_end = .;
  /DISCARD/ : { *(.comment .note* .gnu*) }
}
