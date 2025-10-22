[BITS 32]

global idt_load
extern idtp

section .note.GNU-stack noalloc noexec nowrite progbits

section .text

idt_load:
    lidt [idtp]
    ret