#include <fs/elixir.h>
#include <vga.h>
#include <ide.h>

struct index* create_file(uint8_t drive){
    struct index* in;

    if (drive >= 4) {
        printf("Error: Invalid drive index %d.\n", drive);
        return NULL;
    }

    printf("Allocating super_block struct...\n");
    in = kmalloc(sizeof(struct index));
    if (!in)  {
        printf("Failed to allocate index!\n");
        return NULL;
    }

    memset(in, 0, sizeof(struct index));

    in->type = 1; // Standard txt file
    in->size = 0;
}