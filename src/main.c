#include <gdt.h>
#include <idt.h>
#include <vga.h>
#include <exceptions.h>
#include <irq.h>
#include <timer.h>
#include <mem.h>
#include <ide.h>
#include <fs/elixir.h>

void kmain(void) {
    init_allocator_region(0x00100000, 16 * 1024 * 1024);
    
    printf("Initializing GDT...\n");
    gdt_install(); 
    
    printf("Initializing IDT...\n");
    idt_install();
    
    printf("Installing Exception Handlers...\n");
    exceptions_install();
    
    printf("Installing IRQ Handlers...\n");
    irq_install();
    
    install_irq_handler(0, on_irq0); 
    
    init_timer();
    
    clear_screen();

    asm volatile ("sti");
    printf("Welcome To BinbowsDOS!\n");

    printf("Detecting IDE devices...\n");
    ide_initialize();
    printf("IDE devices detected and initialized.\n");

    while (1) {
        asm volatile ("hlt"); 
    }
}