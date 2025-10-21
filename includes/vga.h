#ifndef VGA_H
#define VGA_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

void clear_screen();
void print_string(const char* str, int row, int col);
void set_cursor(int row, int col);

#endif