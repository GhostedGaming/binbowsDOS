#include <vga.h>
#include <commands.h>
#include <stdarg.h>

#define VGA_MEMORY   ((volatile unsigned short*)0xB8000)
#define VGA_COLOR    0x0F

static int cursor_row = 0;
static int cursor_col = 0;

void set_cursor(int row, int col) {
    cursor_row = row;
    cursor_col = col;
    
    unsigned short position = (row * VGA_WIDTH) + col;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void clear_screen() {
    volatile unsigned short* vga = VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR << 8) | ' ';
    }
    set_cursor(0, 0);
}

static void scroll_screen() {
    volatile unsigned short* vga = VGA_MEMORY;
    
    // Move all lines up by one
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga[i] = vga[i + VGA_WIDTH];
    }
    
    // Clear the last line
    for (int i = VGA_WIDTH * (VGA_HEIGHT - 1); i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (VGA_COLOR << 8) | ' ';
    }
}

static void put_char(char c) {
    volatile unsigned short* vga = VGA_MEMORY;
    
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 4) & ~(4 - 1);
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
        }
    } else {
        int position = cursor_row * VGA_WIDTH + cursor_col;
        vga[position] = (VGA_COLOR << 8) | c;
        cursor_col++;
    }
    
    // Handle line wrap
    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
    
    // Handle screen scroll
    if (cursor_row >= VGA_HEIGHT) {
        scroll_screen();
        cursor_row = VGA_HEIGHT - 1;
        cursor_col = 0;
    }
}

void print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        put_char(str[i]);
    }
    set_cursor(cursor_row, cursor_col);
}

// Helper function to print a string
static void print_str(const char* str) {
    if (!str) {
        str = "(null)";
    }
    while (*str) {
        put_char(*str++);
    }
}

// Helper function to get string length
static int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

// Helper function to print an integer
static void print_int(int num) {
    if (num == 0) {
        put_char('0');
        return;
    }
    
    if (num < 0) {
        put_char('-');
        num = -num;
    }
    
    char buffer[12];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    // Print in reverse
    while (i > 0) {
        put_char(buffer[--i]);
    }
}

// Helper function to print an unsigned integer
static void print_uint(unsigned int num) {
    if (num == 0) {
        put_char('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        put_char(buffer[--i]);
    }
}

// Helper function to print hex
static void print_hex(unsigned int num, int uppercase) {
    const char* digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    
    if (num == 0) {
        put_char('0');
        return;
    }
    
    char buffer[9];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = digits[num % 16];
        num /= 16;
    }
    
    while (i > 0) {
        put_char(buffer[--i]);
    }
}

// Helper function to print pointer
static void print_ptr(void* ptr) {
    unsigned int addr = (unsigned int)ptr;
    put_char('0');
    put_char('x');
    
    const char* digits = "0123456789abcdef";
    char buffer[8];
    
    // Always print 8 hex digits for pointer
    for (int i = 7; i >= 0; i--) {
        buffer[i] = digits[addr & 0xF];
        addr >>= 4;
    }
    
    for (int i = 0; i < 8; i++) {
        put_char(buffer[i]);
    }
}

// Printf implementation
int printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    
    int written = 0;
    
    while (*format) {
        if (*format == '%') {
            format++;
            
            switch (*format) {
                case 'c': {
                    // Character
                    char c = (char)va_arg(args, int);
                    put_char(c);
                    written++;
                    break;
                }
                
                case 's': {
                    // String
                    const char* str = va_arg(args, const char*);
                    print_str(str);
                    written += strlen(str ? str : "(null)");
                    break;
                }
                
                case 'd':
                case 'i': {
                    // Signed integer
                    int num = va_arg(args, int);
                    print_int(num);
                    written++;
                    break;
                }
                
                case 'u': {
                    // Unsigned integer
                    unsigned int num = va_arg(args, unsigned int);
                    print_uint(num);
                    written++;
                    break;
                }
                
                case 'x': {
                    // Hex lowercase
                    unsigned int num = va_arg(args, unsigned int);
                    print_hex(num, 0);
                    written++;
                    break;
                }
                
                case 'X': {
                    // Hex uppercase
                    unsigned int num = va_arg(args, unsigned int);
                    print_hex(num, 1);
                    written++;
                    break;
                }
                
                case 'p': {
                    // Pointer
                    void* ptr = va_arg(args, void*);
                    print_ptr(ptr);
                    written += 10; // "0x" + 8 digits
                    break;
                }
                
                case '%': {
                    // Literal %
                    put_char('%');
                    written++;
                    break;
                }
                
                default: {
                    // Unknown format specifier, print as-is
                    put_char('%');
                    put_char(*format);
                    written += 2;
                    break;
                }
            }
        } else {
            put_char(*format);
            written++;
        }
        
        format++;
    }
    
    set_cursor(cursor_row, cursor_col);
    va_end(args);
    
    return written;
}