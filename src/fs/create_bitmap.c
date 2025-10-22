#include <fs/elixir.h>
#include <mem.h>
#include <ide.h>

struct block_bitmap *create_bitmap(uint8_t drive) {
    extern struct ide_device ide_devices[4];

    if (drive >= 4) {
        printf("Invalid drive index: %u\n", (unsigned)drive);
        return NULL;
    }

    uint32_t size = ide_devices[drive].size;
    if (size == 0) {
        printf("Drive %u reports zero size\n", (unsigned)drive);
        return NULL;
    }

    struct block_bitmap *bb = kmalloc(sizeof(struct block_bitmap));
    if (!bb) {
        printf("Failed to allocate block_bitmap\n");
        return NULL;
    }

    bb->free_count  = 0;
    bb->used_count  = 0;
    bb->total       = size;

    uint8_t buf[512];

    for (uint32_t i = 0; i < size; ++i) {
        if (ide_read_sectors(drive, 1, i, buf) != 0) {
            printf("Failed to read sector %u on drive %u\n", (unsigned)i, (unsigned)drive);
            destroy(bb);
            return NULL;
        }

        bool all_zero = true;
        for (size_t j = 0; j < sizeof(buf); ++j) {
            if (buf[j] != 0) {
                all_zero = false;
                break;
            }
        }

        if (all_zero) {
            bb->free_count++;
        } else {
            bb->used_count++;
        }
    }

    return bb;
}