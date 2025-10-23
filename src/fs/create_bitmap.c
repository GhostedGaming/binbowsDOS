#include <stdbool.h>
#include <fs/elixir.h>
#include <mem.h>
#include <ide.h>
#include <vga.h>

struct block_bitmap *create_bitmap(uint8_t drive) {
    extern struct ide_device ide_devices[4];

    if (drive >= 4) {
        printf("Invalid drive index: %u\n", (unsigned)drive);
        return NULL;
    }

    uint32_t size = ide_devices[drive].Size;
    if (size == 0) {
        printf("Drive %u reports zero size\n", (unsigned)drive);
        return NULL;
    }

    struct block_bitmap *bb = kmalloc(sizeof(struct block_bitmap));
    if (!bb) {
        printf("Failed to allocate block_bitmap\n");
        return NULL;
    }

    uint32_t bitmap_bytes = (size + 7) / 8;
    bb->bitmap = kmalloc(bitmap_bytes);
    if (!bb->bitmap) {
        printf("Failed to allocate bitmap data\n");
        kfree(bb);
        return NULL;
    }

    memset(bb->bitmap, 0, bitmap_bytes);

    bb->total = size;
    bb->free_count = size;
    bb->used_count = 0;

    printf("Bitmap created: %u sectors, %u bytes\n", size, bitmap_bytes);

    return bb;
}

int elixir_write_bitmap(uint8_t drive, struct block_bitmap *bb) {
    if (!bb || !bb->bitmap) return -1;

    uint32_t bitmap_bytes = (bb->total + 7) / 8;
    uint32_t bitmap_sectors = (bitmap_bytes + 511) / 512;

    if (ide_write_sectors_counted(drive, ELIXIR_BITMAP_START_LBA, bitmap_sectors * 512, bb->bitmap) != 0) {
        printf("Error: failed to write bitmap\n");
        return -1;
    }

    return 0;
}

int elixir_read_bitmap(uint8_t drive, struct block_bitmap **bb_out) {
    extern struct ide_device ide_devices[4];

    if (drive >= 4) return -1;

    uint32_t size = ide_devices[drive].Size;
    uint32_t bitmap_bytes = (size + 7) / 8;
    uint32_t bitmap_sectors = (bitmap_bytes + 511) / 512;

    struct block_bitmap *bb = kmalloc(sizeof(struct block_bitmap));
    if (!bb) return -1;

    bb->bitmap = kmalloc(bitmap_sectors * 512);
    if (!bb->bitmap) {
        kfree(bb);
        return -1;
    }

    if (ide_read_sectors(drive, bitmap_sectors, ELIXIR_BITMAP_START_LBA, bb->bitmap) != 0) {
        kfree(bb->bitmap);
        kfree(bb);
        return -1;
    }

    bb->total = size;
    bb->free_count = 0;
    bb->used_count = 0;

    for (uint32_t i = 0; i < size; i++) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        if (bb->bitmap[byte_idx] & (1 << bit_idx)) {
            bb->used_count++;
        } else {
            bb->free_count++;
        }
    }

    *bb_out = bb;
    return 0;
}