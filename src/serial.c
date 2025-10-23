#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <commands.h>
#include <serial.h>

#define COM1_PORT 0x3F8 /* standard COM1 base port */

int serial_received(void) {
    return inb(COM1_PORT + 5) & 0x01; /* Data Ready (DR) */
}

char serial_read(void) {
    while (!serial_received()) { __asm__ volatile ("pause"); }
    return (char)inb(COM1_PORT);
}

int serial_is_transmit_empty(void) {
    return inb(COM1_PORT + 5) & 0x20; /* Transmitter holding register empty (THRE) */
}

void serial_write(char c) {
    while (!serial_is_transmit_empty()) { __asm__ volatile ("pause"); }
    outb(COM1_PORT, (uint8_t)c);
}

void serial_write_string(const char *s) {
    if (!s) return;
    while (*s) serial_write(*s++);
}

static void serial_putnum_unsigned(unsigned long num, int base, int uppercase) {
    char buf[65];
    const char *digits_l = "0123456789abcdef";
    const char *digits_u = "0123456789ABCDEF";
    const char *digits = uppercase ? digits_u : digits_l;
    int i = 0;
    if (num == 0) { serial_write('0'); return; }
    while (num && i < (int)sizeof(buf)-1) {
        buf[i++] = digits[num % base];
        num /= base;
    }
    while (i--) serial_write(buf[i]);
}

static void serial_putnum_signed(long num, int base) {
    if (num < 0) {
        serial_write('-');
        serial_putnum_unsigned((unsigned long)(-num), base, 0);
    } else {
        serial_putnum_unsigned((unsigned long)num, base, 0);
    }
}

void serial_vserial_printf(const char *fmt, va_list ap) {
    if (!fmt) return;
    for (; *fmt; ++fmt) {
        if (*fmt != '%') { serial_write(*fmt); continue; }
        ++fmt;
        if (!*fmt) break;
        switch (*fmt) {
            case 's': {
                const char *s = va_arg(ap, const char *);
                serial_write_string(s ? s : "(null)");
            } break;
            case 'c': {
                char c = (char)va_arg(ap, int);
                serial_write(c);
            } break;
            case 'd':
            case 'i': {
                int v = va_arg(ap, int);
                serial_putnum_signed((long)v, 10);
            } break;
            case 'u': {
                unsigned int v = va_arg(ap, unsigned int);
                serial_putnum_unsigned((unsigned long)v, 10, 0);
            } break;
            case 'x': {
                unsigned int v = va_arg(ap, unsigned int);
                serial_putnum_unsigned((unsigned long)v, 16, 0);
            } break;
            case 'X': {
                unsigned int v = va_arg(ap, unsigned int);
                serial_putnum_unsigned((unsigned long)v, 16, 1);
            } break;
            case 'p': {
                void *p = va_arg(ap, void *);
                serial_write('0'); serial_write('x');
                serial_putnum_unsigned((unsigned long)(uintptr_t)p, 16, 0);
            } break;
            case '%':
                serial_write('%');
                break;
            default:
                serial_write('%');
                serial_write(*fmt);
                break;
        }
    }
}

void serial_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    serial_vserial_printf(fmt, ap);
    va_end(ap);
}

void serial_init(void) {
    outb(COM1_PORT + 1, 0x00);    /* Disable all interrupts */
    outb(COM1_PORT + 3, 0x80);    /* Enable DLAB (set baud rate divisor) */
    outb(COM1_PORT + 0, 0x01);    /* Divisor low byte (115200) */
    outb(COM1_PORT + 1, 0x00);    /* Divisor high byte */
    outb(COM1_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(COM1_PORT + 2, 0xC7);    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(COM1_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
}