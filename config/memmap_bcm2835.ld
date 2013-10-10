/**
 *******************************************************************************
 * @file    memmap_bcm2835.ld
 * @author  Olli Vanhoja
 * @brief   linker script for BCM2835.
 *******************************************************************************
 */
ENTRY(Start)

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

    .hw_preinit_array :
    {  
        PROVIDE_HIDDEN (__hw_preinit_array_start = .);
        KEEP (*(SORT(.hw_preinit_array.*)))
        KEEP (*(.hw_preinit_array*))
        PROVIDE_HIDDEN (__hw_preinit_array_end = .);
    }

    _cinit_data_start = .;
    .init_array :
    { 
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP (*(SORT(.init_array.*)))
        KEEP (*(.init_array*))
        PROVIDE_HIDDEN (__init_array_end = .);
    }

    .hw_postinit_array :
    {
        PROVIDE_HIDDEN (__hw_postinit_array_start = .);
        KEEP (*(SORT(.hw_postinit_array.*)))
        KEEP (*(.hw_postinit_array*))
        PROVIDE_HIDDEN (__hw_postinit_array_end = .);
    } 

    .fini_array :
    { 
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP (*(.fini_array*))
        KEEP (*(SORT(.fini_array.*)))
        PROVIDE_HIDDEN (__fini_array_end = .);
    }
    . = ALIGN(4096); /* align to page size */
    _cinit_data_end = .;

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
