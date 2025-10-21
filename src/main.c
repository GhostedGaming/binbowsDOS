#include <vga.h>

// Main kernel entry point called by the bootloader
void kmain() {
    clear_screen();
    print_string("Welcome to binbowsDOS!", 0, 0);

    while (1) {
        asm volatile ("hlt");
    }
}