#include <gdt.h>
#include <idt.h>
#include <vga.h>
#include <exceptions.h>
#include <irq.h>
#include <timer.h>

void kernel_main(void) {
    clear_screen();
    printf("Welcome to binbowsDOS!\n");
    printf("=====================\n\n");
    
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
    
    printf("\nSystem initialized successfully!\n");
    
    asm volatile ("sti");

    printf("Entering idle loop...\n");
    
    for(;;) {
        asm volatile ("hlt"); 
    }
}