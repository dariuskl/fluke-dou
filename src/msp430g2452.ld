OUTPUT_ARCH(msp430)
ENTRY(on_reset)

MEMORY {
    ram (rw) : ORIGIN = 0x0200, LENGTH = 256
    info (r) : ORIGIN = 0x1000, LENGTH = 256
    rom (rx) : ORIGIN = 0xe000, LENGTH = 8192 - 64
    vectors : ORIGIN = 0xffc0, LENGTH = 64
}

SECTIONS {
  .vectors :
  {
    KEEP (*(.vectors))
  } > vectors

  .rodata :
  {
    . = ALIGN(2);
    _preinit_array_start = .;
    KEEP(*(.preinit_array))
    _preinit_array_end = .;
    . = ALIGN(2);
    _init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    _init_array_end = .;
    . = ALIGN(2);
    _fini_array_start = .;
    KEEP(*(.fini_array))
    KEEP(*(SORT(.fini_array.*)))
    _fini_array_end = .;
    . = ALIGN(2);
    *(.rodata .rodata.*)
  } > rom

  .text :
  {
    . = ALIGN(2);
    *(.text)
  } > rom

  .data :
  {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.* .gnu.linkonce.d.*)
    . = ALIGN(2);
    _edata = .;
  } > ram AT>rom

  PROVIDE(_sidata = LOADADDR(.data));

  .bss :
  {
    . = ALIGN(2);
    _sbss = .;
    *(.bss)
    . = ALIGN(2);
    _ebss = .;
  } > ram

  .stack (ORIGIN(ram) + LENGTH(ram)) :
  {
    _stack = .;
    *(.stack)
  }

}

PROVIDE(P1IN  = 0x20);
PROVIDE(P1OUT = 0x21);
PROVIDE(P1DIR = 0x22);
PROVIDE(P1IFG = 0x23);
PROVIDE(P1IES = 0x24);
PROVIDE(P1IE  = 0x25);
PROVIDE(P1REN = 0x27);

PROVIDE(P2IN  = 0x28);
PROVIDE(P2OUT = 0x29);
PROVIDE(P2DIR = 0x2a);
PROVIDE(P2IFG = 0x2b);
PROVIDE(P2IES = 0x2c);
PROVIDE(P2IE  = 0x2d);
PROVIDE(P2SEL = 0x2e);
PROVIDE(P2REN = 0x2f);

PROVIDE(BCSCTL3 = 0x53);
PROVIDE(DCOCTL  = 0x56);
PROVIDE(BCSCTL1 = 0x57);

PROVIDE(WDTCTL = 0x120);

PROVIDE(CAL_DCO_16MHz = 0x10f6 + 2);
PROVIDE(CAL_BC1_16MHz = 0x10f6 + 3);
