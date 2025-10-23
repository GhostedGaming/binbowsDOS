#include <fs/elixir.h>
#include <vga.h>
#include <ide.h>
#include <mem.h>

struct index* create_file(uint8_t drive) {
    struct index* in;

    if (drive >= 4) {
        printf("Error: Invalid drive index %d.\n", drive);
        return NULL;
    }

    printf("Allocating index struct...\n");
    in = kmalloc(sizeof(struct index));
    if (!in)  {
        printf("Failed to allocate index!\n");
        return NULL;
    }

    memset(in, 0, sizeof(struct index));

    in->type = 1;
    in->size = 0;
    in->first_block = 0;
    in->time_stamp = 0;

    return in;
}