ORG 0x7C00

KERNEL_OFFSET equ 0x00100000            ; Where to load kernel in memory
MAX_KERNEL_SECTORS equ 2048             ; Maximum sectors to attempt (1MB)

start: 
    mov [boot_drive], dl                ; Save boot drive number (BIOS passes it in DL)
    
    xor ax, ax                          ; Clear AX register
    mov ds, ax                          ; Set DS segment to 0
    mov es, ax                          ; Set ES segment to 0
    mov ss, ax                          ; Set SS segment to 0
    mov sp, 0x7C00                      ; Set stack pointer

    ; Print boot message
    mov si, boot_msg
    call print_string

    ; Reset disk system first
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13

    ; Load kernel from disk - read until we hit an error or max sectors
    mov bx, 0x1000                      ; Load kernel to 0x1000 temporarily
    mov cl, 0x02                        ; Start from sector 2 (after bootloader)
    mov dh, 0                           ; Head 0
    mov ch, 0                           ; Cylinder 0
    mov di, MAX_KERNEL_SECTORS          ; Maximum sectors to read
    call load_kernel_loop

    mov si, load_success_msg
    call print_string

    lgdt [gdt_descriptor]               ; Load the GDT

    ; Switch to protected mode
    cli                                 ; Disable interrupts
    mov eax, cr0                        ; Get control register 0
    or eax, 1                           ; Set PE bit (bit 0)
    mov cr0, eax                        ; Update control register 0
    jmp CODE_SEG:protected_mode_start   ; Far jump to flush the prefetch queue

; Load kernel in a loop, reading sectors one at a time
[BITS 16]
load_kernel_loop:
    pusha
    mov si, loading_msg
    call print_string
    popa

    mov word [sectors_read], 0          ; Initialize sector counter

.read_loop:
    cmp di, 0
    je .done
    
    pusha
    
    ; Read one sector at a time
    mov ah, 0x02                        ; BIOS read sector function
    mov al, 1                           ; Read 1 sector
    mov dl, [boot_drive]                ; Drive number
    ; CH, CL, DH already set
    ; BX already points to buffer
    
    int 0x13                            ; BIOS disk interrupt
    jc .check_if_end                    ; Jump if carry flag set (could be end of data)
    
    ; Success - increment counter
    inc word [sectors_read]
    
    ; Move to next sector
    add bx, 512                         ; Move buffer pointer forward
    inc cl                              ; Next sector
    
    ; Check if we need to move to next track
    cmp cl, 19                          ; 18 sectors per track (common for floppies)
    jl .no_track_change
    
    mov cl, 1                           ; Reset to sector 1
    inc dh                              ; Next head
    
    cmp dh, 2                           ; 2 heads (0 and 1)
    jl .no_track_change
    
    mov dh, 0                           ; Reset head
    inc ch                              ; Next cylinder

.no_track_change:
    popa
    dec di                              ; One less sector to read
    jmp .read_loop

.check_if_end:
    ; Check if we've read at least some sectors
    ; If we have, this might just be the end of the kernel
    cmp word [sectors_read], 0
    je .disk_error                      ; No sectors read = real error
    
    ; Otherwise, assume we've hit the end of the kernel
    popa
    ret

.done:
    ret

.disk_error:
    popa
    mov si, disk_error_msg
    call print_string
    mov al, ah
    call print_hex
    jmp $

print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    int 0x10
    jmp .loop
.done:
    popa
    ret

print_hex:
    pusha
    mov cx, ax
    shr al, 4
    call print_hex_digit
    mov ax, cx
    and al, 0x0F
    call print_hex_digit
    popa
    ret

print_hex_digit:
    cmp al, 0x0A
    jl .digit
    add al, 'A' - 10
    jmp .print
.digit:
    add al, '0'
.print:
    mov ah, 0x0E
    int 0x10
    ret

boot_msg: db "Booting binbowsDOS...", 13, 10, 0
loading_msg: db "Loading kernel...", 13, 10, 0
load_success_msg: db "Kernel loaded!", 13, 10, 0
disk_error_msg: db "Disk error: 0x", 0
boot_drive: db 0
sectors_read: dw 0

[BITS 32]
protected_mode_start:
    mov ax, DATA_SEG                    ; Load data segment selector
    mov ds, ax                          ; Set DS to data segment
    mov es, ax                          ; Set ES to data segment
    mov ss, ax                          ; Set SS to data segment
    mov fs, ax
    mov gs, ax
    mov esp, 0x9FC00                    ; Set stack pointer for protected mode

    ; TEST: Write 'P' to screen to prove we're in protected mode
    mov dword [0xB8000], 0x0F500F50     ; 'PP' in white on black

    ; Copy kernel from 0x1000 to 0x100000 (1MB)
    ; Copy maximum possible size (we don't know exact size)
    mov esi, 0x1000                     ; Source: temporary location
    mov edi, KERNEL_OFFSET              ; Destination: 1MB
    mov ecx, (MAX_KERNEL_SECTORS * 512) / 4 ; Convert bytes to dwords
    rep movsd                           ; Copy dwords

    ; TEST: Write 'K' to screen before jumping to kernel
    mov dword [0xB8004], 0x0F4B0F4B     ; 'KK' in white on black

    jmp KERNEL_OFFSET                   ; Jump to kernel entry point at 1MB

[BITS 16]
gdt_start:
    dq 0x0000000000000000               ; Null Descriptor

gdt_code:
    dw 0xFFFF                           ; Limit (bits 0-15)
    dw 0x0000                           ; Base (bits 0-15)
    db 0x00                             ; Base (bits 16-23)
    db 10011010b                        ; Access byte
    db 11001111b                        ; Flags + Limit
    db 0x00                             ; Base (bits 24-31)

gdt_data:
    dw 0xFFFF                           ; Limit (bits 0-15)
    dw 0x0000                           ; Base (bits 0-15)
    db 0x00                             ; Base (bits 16-23)
    db 10010010b                        ; Access byte
    db 11001111b                        ; Flags + Limit
    db 0x00                             ; Base (bits 24-31)

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1          ; Size of GDT (limit)
    dd gdt_start                        ; Address of GDT

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510 - ($ - $$) db 0               ; Pad to 512
dw 0xAA55                               ; Boot sector signature