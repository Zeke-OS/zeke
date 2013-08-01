/**
 *******************************************************************************
 * @file    memmap_bcm2835.ld
 * @author  Olli Vanhoja
 * @brief   linker script for BCM2835.
 *******************************************************************************
 */
ENTRY(Start)

/* TODO sections for cinit */

SECTIONS
{
    /* Execution starts at LOADER_ADDR. */
    . = 0x8000;
    _start = .;
    _text_start = .;
    .text :
    {
        KEEP(*(.text.boot))
        *(.text)
    }
    . = ALIGN(4096); /* align to page size */
    _text_end = .;
 
    _rodata_start = .;
    .rodata :
    {
        *(.rodata)
    }
    . = ALIGN(4096); /* align to page size */
    _rodata_end = .;
 
    _data_start = .;
    .data :
    {
        *(.data)
    }
    . = ALIGN(4096); /* align to page size */
    _data_end = .;
 
    _bss_start = .;
    .bss :
    {
        bss = .;
        *(.bss)
    }
    . = ALIGN(4096); /* align to page size */
    _bss_end = .;
    _end = .;
}
