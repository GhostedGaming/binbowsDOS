[BITS 32]

global irq0, irq1, irq2, irq3, irq4, irq5, irq6, irq7
global irq8, irq9, irq10, irq11, irq12, irq13, irq14, irq15

extern irq_handler

section .note.GNU-stack noalloc noexec nowrite progbits

section .text

; Macro for IRQ handlers
%macro IRQ 2
global irq%1
irq%1:
    cli
    push 0              ; Dummy error code
    push %2             ; IRQ number (32 + IRQ line)
    jmp irq_common_stub
%endmacro

; Hardware IRQs (remapped to 32-47)
IRQ 0,  32    ; Timer
IRQ 1,  33    ; Keyboard
IRQ 2,  34    ; Cascade
IRQ 3,  35    ; COM2
IRQ 4,  36    ; COM1
IRQ 5,  37    ; LPT2
IRQ 6,  38    ; Floppy
IRQ 7,  39    ; LPT1
IRQ 8,  40    ; CMOS RTC
IRQ 9,  41    ; Free
IRQ 10, 42    ; Free
IRQ 11, 43    ; Free
IRQ 12, 44    ; PS/2 Mouse
IRQ 13, 45    ; FPU
IRQ 14, 46    ; Primary ATA
IRQ 15, 47    ; Secondary ATA

; Common IRQ stub
irq_common_stub:
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push stack pointer (contains all register state)
    mov eax, esp
    push eax
    
    ; Call C IRQ handler
    call irq_handler
    
    ; Pop the pushed esp
    pop eax
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret