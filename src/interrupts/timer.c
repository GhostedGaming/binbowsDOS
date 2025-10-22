#include <timer.h>
#include <commands.h>
#include <pic.h>
#include <vga.h>
#include <stdint.h>

static volatile uint64_t timer_ticks = 0;

void on_irq0(void) {
    timer_ticks++;
}

void timer_wait(uint32_t ticks) {
    uint64_t start_ticks = timer_ticks;
    while (timer_ticks < start_ticks + ticks) {
        asm volatile ("hlt"); 
    }
}

void init_timer(void) {
    // Set PIT frequency to ~100Hz (1193180 / 11932 ≈ 100)
    uint16_t divisor = 11932;

    // Command Port (0x43): Channel 0, Access LOBYTE/HIBYTE, Mode 3 (Square Wave), Binary
    outb(0x43, 0x36); 
    // Data Port (0x40): Send low byte
    outb(0x40, divisor & 0xFF);
    // Data Port (0x40): Send high byte
    outb(0x40, (divisor >> 8) & 0xFF);
    
    printf("PIT: Timer initialized at ~100Hz\n");
}

uint64_t get_timer_ticks(void) {
    return timer_ticks;
}