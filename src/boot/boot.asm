ORG 0x7C00
BITS 16

KERNEL_OFFSET equ 0x00100000            ; Where kernel will be in memory (1MB)

start: 
    cli                                 ; Disable interrupts during setup
    xor ax, ax                          ; Clear AX
    mov ds, ax                          ; Set DS = 0
    mov es, ax                          ; Set ES = 0
    mov ss, ax                          ; Set SS = 0
    mov sp, 0x7C00                      ; Stack grows down from bootloader
    sti                                 ; Re-enable interrupts
    
    mov [boot_drive], dl                ; Save boot drive (BIOS passes in DL)
    
    ; Visual checkpoint 0
    mov word [0xB8000], 0x0F30          ; '0' in white

    ; Print boot message
    mov si, msg_boot
    call print_string

    ; Reset disk system
    mov ah, 0x00
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    
    ; Visual checkpoint 1
    mov word [0xB8002], 0x0F31          ; '1'
    
    mov bx, 0x1000                      ; Start loading at 0x1000
    mov cl, 2                           ; Start at sector 2
    mov ch, 0                           ; Cylinder 0
    mov dh, 0                           ; Head 0
    xor di, di                          ; Sector counter = 0

.load_loop:
    mov ah, 0x02                        ; BIOS read sectors
    mov al, 1                           ; Read 1 sector at a time
    mov dl, [boot_drive]
    int 0x13
    jc .load_done                       ; If error, we're done (probably no more sectors)
    
    inc di                              ; Increment sector counter
    add bx, 0x200                       ; Move buffer forward 512 bytes
    
    ; Check if we've filled segment (BX would wrap)
    cmp bx, 0xF000
    jae .load_done                      ; Stop if we're near segment boundary
    
    inc cl                              ; Next sector
    cmp cl, 19                          ; 18 sectors per track (floppy standard)
    jl .load_loop
    
    mov cl, 1                           ; Reset to sector 1
    inc dh                              ; Next head
    cmp dh, 2                           ; Check if we need next cylinder
    jl .load_loop
    
    mov dh, 0                           ; Reset head
    inc ch                              ; Next cylinder
    cmp ch, 80                          ; Limit cylinders (safety)
    jl .load_loop

.load_done:
    ; Visual checkpoint 2
    mov word [0xB8004], 0x0F32          ; '2'
    
    ; Print sector count
    mov ax, di
    call print_hex_word

    ; Load GDT
    lgdt [gdt_descriptor]
    
    ; Visual checkpoint 3
    mov word [0xB8006], 0x0F33          ; '3'

    ; Enter protected mode
    cli                                 ; Disable interrupts
    mov eax, cr0
    or eax, 1                           ; Set PE bit
    mov cr0, eax
    
    ; Far jump to protected mode code
    jmp CODE_SEG:protected_mode_start

disk_error:
    mov si, msg_disk_error
    call print_string
    mov al, ah                          ; Error code in AH
    call print_hex
    jmp $

; Print null-terminated string at DS:SI
print_string:
    pusha
.loop:
    lodsb                               ; Load byte from [DS:SI] into AL
    test al, al                         ; Check for null terminator
    jz .done
    mov ah, 0x0E                        ; BIOS teletype
    mov bh, 0                           ; Page 0
    int 0x10                            ; Print character
    jmp .loop
.done:
    popa
    ret

; Print byte in AL as hex
print_hex:
    pusha
    mov cx, ax                          ; Save AL
    shr al, 4                           ; High nibble
    call print_hex_digit
    mov ax, cx
    and al, 0x0F                        ; Low nibble
    call print_hex_digit
    popa
    ret

; Print word in AX as hex
print_hex_word:
    pusha
    mov cx, ax
    mov al, ah                          ; High byte
    call print_hex
    mov ax, cx
    call print_hex                      ; Low byte
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

; Data
msg_boot:        db 'binbowsDOS Booting...', 13, 10, 0
msg_disk_error:  db 'DISK ERROR: 0x', 0
boot_drive:      db 0

BITS 32
protected_mode_start:
    ; Set up segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000                    ; Set stack pointer
    
    ; Copy kernel from 0x1000 to 0x100000 (1MB)
    ; Copy up to 60KB (safe amount we loaded)
    mov esi, 0x1000                     ; Source
    mov edi, KERNEL_OFFSET              ; Destination
    mov ecx, 15360                      ; 60KB / 4 = 15360 dwords
    rep movsd                           ; Copy
    
    ; Jump to kernel
    call KERNEL_OFFSET                  ; Call kernel
    
    ; If kernel returns, halt
    mov dword [0xB8010], 0x0F580F58     ; 'XX' = kernel returned
    jmp $

; ============================================
; GDT (Global Descriptor Table)
; ============================================
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