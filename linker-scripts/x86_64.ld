OUTPUT_FORMAT(elf32-i386)
ENTRY(kmain)

SECTIONS
{
    . = 0x00100000;                     /* 1MB - standard kernel load address */

    .text ALIGN(4K) : {
        *(.multiboot)                   /* Multiboot header first (if you add one later) */
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4K) : {               /* Separate rodata for better organization */
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) : {
        __bss_start = .;                /* Mark BSS boundaries for zeroing */
        *(COMMON)
        *(.bss)
        *(.bss.*)
        __bss_end = .;
    }

    . = ALIGN(4K);
    __kernel_end = .;                   /* Mark end of kernel for memory management */

    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note.*)
        *(.comment)
    }
}