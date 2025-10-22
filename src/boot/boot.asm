ORG 0x7C00
BITS 16

KERNEL_LOAD_SEG equ 0x0050
KERNEL_FINAL_OFFSET equ 0x00100000

start: 
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    mov si, msg_boot
    call print_string

    mov ah, 0x08                        ; Get drive parameters
    mov dl, [boot_drive]
    xor di, di
    int 0x13
    jc disk_error

    mov al, ch
    mov ah, cl
    shr ah, 6
    inc ax
    mov [cylinders], ax

    and cl, 0x3F
    mov [sectors_per_track], cl
    inc dh
    mov [heads], dh

    mov ah, 0x00                        ; Reset disk
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    xor bx, bx
    mov cx, 0x0002
    mov dh, 0
    xor edi, edi

.load_loop:
    pusha
    mov ah, 0x02
    mov al, 1
    mov dl, [boot_drive]
    int 0x13
    popa
    jc .load_complete

    inc edi
    add bx, 0x200
    jnc .no_wrap
    mov ax, es
    add ax, 0x1000
    mov es, ax
    xor bx, bx
.no_wrap:

    inc cl
    cmp cl, [sectors_per_track]
    jbe .load_loop
    
    mov cl, 1
    inc dh
    cmp dh, [heads]
    jb .load_loop
    
    mov dh, 0
    inc ch
    jnz .load_loop

.load_complete:
    mov si, msg_loaded
    call print_string

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp CODE_SEG:protected_mode_start

disk_error:
    mov si, msg_error
    call print_string
    jmp $

print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

msg_boot:       db 'binbowsDOS Booting...', 13, 10, 0
msg_loaded:     db 'Loaded!', 13, 10, 0
msg_error:      db 'ERR', 0
boot_drive:     db 0
cylinders:      dw 0
sectors_per_track: db 0
heads:          db 0

BITS 32
protected_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    mov esi, 0x500
    mov edi, KERNEL_FINAL_OFFSET
    mov ecx, 30720                      ; Copy ~120KB (plenty for kernel)
    rep movsd

    call KERNEL_FINAL_OFFSET

    jmp $

BITS 16
gdt_start:
    dq 0x0000000000000000

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510-($-$$) db 0
dw 0xAA55