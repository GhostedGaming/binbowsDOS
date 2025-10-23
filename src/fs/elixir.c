#include <stdint.h>
#include <mem.h>
#include <ide.h>
#include <fs/elixir.h>
#include <vga.h>

int elixir_format(uint8_t drive) {
    struct super_block *sb = create_super(drive);
    if (!sb) {
        printf("Error: create_super failed\n");
        return -1;
    }

    size_t sb_bytes = sizeof(struct super_block);
    if (ide_write_sectors_counted(drive, ELIXIR_SUPERBLOCK_LBA, sb_bytes, (const void *)sb) != 0) {
        printf("Error: failed to write superblock to drive %u\n", (unsigned)drive);
        kfree(sb);
        return -1;
    }

    printf("Superblock written to LBA %u\n", ELIXIR_SUPERBLOCK_LBA);

    struct block_bitmap *bb = create_bitmap(drive);
    if (!bb) {
        printf("Error: failed to create bitmap\n");
        kfree(sb);
        return -1;
    }

    if (elixir_write_bitmap(drive, bb) != 0) {
        printf("Error: failed to write bitmap\n");
        kfree(bb->bitmap);
        kfree(bb);
        kfree(sb);
        return -1;
    }

    printf("Bitmap written to LBA %u\n", sb->s_bitmap_start_lba);

    kfree(bb->bitmap);
    kfree(bb);
    kfree(sb);

    printf("Elixir filesystem formatted successfully on drive %u\n", (unsigned)drive);
    return 0;
}

int elixir_mount(uint8_t drive, struct super_block **sb_out) {
    if (drive >= 4) {
        printf("Error: Invalid drive index %u\n", (unsigned)drive);
        return -1;
    }

    struct super_block *sb = kmalloc(sizeof(struct super_block));
    if (!sb) {
        printf("Error: failed to allocate superblock\n");
        return -1;
    }

    if (ide_read_sectors(drive, 1, ELIXIR_SUPERBLOCK_LBA, sb) != 0) {
        printf("Error: failed to read superblock from drive %u\n", (unsigned)drive);
        kfree(sb);
        return -1;
    }

    if (sb->s_magic != ELIXIR_MAGIC) {
        printf("Error: invalid magic number 0x%X (expected 0x%X)\n", sb->s_magic, ELIXIR_MAGIC);
        kfree(sb);
        return -1;
    }

    printf("Elixir filesystem mounted on drive %u\n", (unsigned)drive);
    printf("  Block size: %u bytes\n", sb->s_block_size);
    printf("  Total blocks: %u\n", sb->s_total_blocks);
    printf("  Free blocks: %u\n", sb->s_free_blocks);

    *sb_out = sb;
    return 0;
}