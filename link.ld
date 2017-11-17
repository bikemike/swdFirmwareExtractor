/*
 * Copyright (C) 2017 Obermaier Johannes
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

/* entry point address for ELF */
ENTRY(Reset_Handler)

/* place stack at end of 8K SRAM */
_estack = 0x20002000;

/* Specify the memory areas */
MEMORY
{
  FLASH (rx)	: ORIGIN = 0x08000000, LENGTH = 64K
  RAM (xrw) 	: ORIGIN = 0x20000000, LENGTH = 8K
}

SECTIONS
{
 .text :
  {
    . = ALIGN(4);
    *(.isr_vector)
    *(.text)
    *(.text*)
    . = ALIGN(4);
    _etext = .;
  } >FLASH

  .rodata :
  {
    . = ALIGN(8);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(8);
  } >FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(8);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(8);
    _edata = .;
  } >RAM AT> FLASH

  . = ALIGN(8);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(8);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

}
