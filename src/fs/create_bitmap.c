#include <fs/elixir.h>
#include <mem.h>
#include <ide.h>

struct block_bitmap *create_bitmap(uint8_t drive) {
    extern struct ide_device ide_devices[4];
    uint32_t size = ide_device[drive].size;
    struct block_bitmap bb;
    if (drive > 4) {
        printf("Invalid drive index: %d", drive);
        return -1;
    }

    bb = kmalloc(sizeof(struct free_bitmap));
    if (!bb) {
        printf("Failed to allocate free bitmap");
        return -1;
    }

    bb->free_count = NULL; // TODO
    bb->used_count = NULL; // TODO: have the amount of free and  used
    bb->total = size;
}