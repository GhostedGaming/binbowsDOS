#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

// Basic VGA functions
void set_cursor(int row, int col);
void clear_screen(void);
void print(const char* str);
int printf(const char* format, ...);

#endif // VGA_H