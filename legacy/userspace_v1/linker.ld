ENTRY(_start)

SECTIONS
{
    . = 0x20000000; /* Load at 512MB to avoid Kernel Heap at 1GB (0x40000000) */
 
    .text : {
        *(.text)
        *(.rodata)
    }
 
    .data : {
        *(.data)
    }
 
    .bss : {
        *(.bss)
    }
}
