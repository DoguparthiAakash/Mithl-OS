/* linker/linker.ld - Link at 1 MiB with Multiboot1 header */
ENTRY(start)

SECTIONS
{
  /* Load the kernel at 1 MiB */
  . = 1M;

  .multiboot :
  {
    *(.multiboot)
  }

  .text ALIGN(4K) :
  {
    *(.text*)
  }

  .rodata ALIGN(4K) :
  {
    *(.rodata*)
  }

  .data ALIGN(4K) :
  {
    *(.data*)
  }

  /* C++ constructors */
  .init_array ALIGN(4K) :
  {
      start_ctors = .;
      KEEP(*(.init_array))
      KEEP(*(SORT(.init_array.*)))
      end_ctors = .;
  }
  
  /* C++ destructors */
  .fini_array ALIGN(4K) :
  {
      start_dtors = .;
      KEEP(*(.fini_array))
      KEEP(*(SORT(.fini_array.*)))
      end_dtors = .;
  }

  .bss ALIGN(4K) :
  {
    *(COMMON)
    *(.bss*)
  }

  /* Define end of kernel symbol */
  _kernel_end = .;
}
