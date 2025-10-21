OUTPUT_FORMAT(elf32-i386)
ENTRY(kmain)

SECTIONS
{
    . = 0x00100000;  /* 1MB - standard kernel load address */

    .text ALIGN(4K) : {
        *(.text)
        *(.text.*)
        *(.rodata)      /* Put rodata in text section */
        *(.rodata.*)
    }

    .data ALIGN(4K) : {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /DISCARD/ : {
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.note.*)
        *(.comment)
    }
}