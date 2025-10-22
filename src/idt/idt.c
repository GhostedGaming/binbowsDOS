#include <idt.h>
#include <vga.h>
#include <commands.h>
#include <irq.h>
#include <pic.h>

struct idt_entry idt[256];
struct idt_ptr idtp;

struct regs {
    unsigned int gs, fs, es, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, useresp, ss;
};

static void (*irq_handlers[16])(void) = {0};

extern void idt_load(unsigned int);

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
    idt[num].base_high = (base >> 16) & 0xFFFF;
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (unsigned int)&idt;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    idt_load((unsigned int)&idtp);
}

void irq_install() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20); // Master PIC starts at 0x20 (32)
    outb(0xA1, 0x28); // Slave PIC starts at 0x28 (40)
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);

    // Set IRQ gates (32-47) to the ASM stubs
    // The ASM stubs will call the general irq_handler C function
    idt_set_gate(32, (unsigned long)irq0, 0x08, 0x8E);
    idt_set_gate(33, (unsigned long)irq1, 0x08, 0x8E);
    idt_set_gate(34, (unsigned long)irq2, 0x08, 0x8E);
    idt_set_gate(35, (unsigned long)irq3, 0x08, 0x8E);
    idt_set_gate(36, (unsigned long)irq4, 0x08, 0x8E);
    idt_set_gate(37, (unsigned long)irq5, 0x08, 0x8E);
    idt_set_gate(38, (unsigned long)irq6, 0x08, 0x8E);
    idt_set_gate(39, (unsigned long)irq7, 0x08, 0x8E);
    idt_set_gate(40, (unsigned long)irq8, 0x08, 0x8E);
    idt_set_gate(41, (unsigned long)irq9, 0x08, 0x8E);
    idt_set_gate(42, (unsigned long)irq10, 0x08, 0x8E);
    idt_set_gate(43, (unsigned long)irq11, 0x08, 0x8E);
    idt_set_gate(44, (unsigned long)irq12, 0x08, 0x8E);
    idt_set_gate(45, (unsigned long)irq13, 0x08, 0x8E);
    idt_set_gate(46, (unsigned long)irq14, 0x08, 0x8E);
    idt_set_gate(47, (unsigned long)irq15, 0x08, 0x8E);
}

void install_irq_handler(int n, void (*handler)(void)) {
    if (n < 16) {
        irq_handlers[n] = handler;
        pic_enable_irq((uint8_t)n);
        printf("Installed IRQ handler for IRQ %d\n", n);
    }
}

void irq_handler(struct regs *r) {
    int irq = r->int_no - 32;

    // Check if we have a custom handler for this IRQ
    if (irq >= 0 && irq < 16 && irq_handlers[irq]) {
        irq_handlers[irq](); // Call the device-specific C handler (e.g., on_irq0)
    }

    // Send EOI to PICs
    if (r->int_no >= 40) { // IRQ 8-15 (Slave PIC)
        outb(0xA0, 0x20); 
    }
    outb(0x20, 0x20); // Master PIC
}