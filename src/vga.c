#include <vga.h>
#include <commands.h>

#define VGA_MEMORY   ((volatile unsigned short*)0xB8000)
#define VGA_COLOR    0x0F

void set_cursor(int row, int col) {
    unsigned short position = (row * VGA_WIDTH) + col;

    outb(0x3D4, 14);
    outb(0x3D5, (position >> 8) & 0xFF);
    outb(0x3D4, 15);
    outb(0x3D5, position & 0xFF);
}

void clear_screen() {
    volatile unsigned short* vga = VGA_MEMORY;
    set_cursor(0, 0);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR << 8) | ' ';
    }
}

void print_string(const char* str, int row, int col) {
    volatile unsigned short* vga = VGA_MEMORY;
    int pos = row * VGA_WIDTH + col;
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        vga[pos + i] = (VGA_COLOR << 8) | str[i];
    }

    set_cursor(row, col + i);
}