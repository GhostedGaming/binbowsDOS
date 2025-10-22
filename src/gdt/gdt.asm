bits 32
global gdt_flush
extern gp

section .text
gdt_flush:
    lgdt [gp]

    jmp 0x08:flush_continue

flush_continue:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret
