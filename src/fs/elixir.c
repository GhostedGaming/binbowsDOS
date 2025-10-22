#include <fs/elixir.h>
#include <vga.h>
#include <mem.h>
#include <ide.h>

int elixir_format(uint8_t drive) {
    struct super_block* sb = create_super(drive);
    if (sb == NULL) {
        printf("Error: Failed to initialize superblock.\n");
        return -1;
    }

    printf("Formatting Elixir filesystem on drive %d...\n", drive);

    uint8_t sectors_to_write = (sizeof(struct super_block) + 511) / 512;
    if (sectors_to_write < 1) sectors_to_write = 1;

    printf("Writing superblock (%u bytes, %u sectors) to disk...\n",
        sizeof(struct super_block), sectors_to_write);

    int result = ide_write_sectors(drive, 2, sizeof(struct super_block),(void*)sb);

    if (result != 0) {
        printf("Error: Failed to write superblock to disk.\n");
        destroy(sb);
        return -1;
    }

    printf("Verifying written superblock...\n");
    struct super_block* sb_verify = kmalloc(sizeof(struct super_block));
    if (!sb_verify) {
        printf("Error: Failed to allocate verification buffer.\n");
        destroy(sb);
        return -1;
    }

    result = ide_read_sectors(drive, 2, sectors_to_write, (void*)sb_verify);
    if (result != 0) {
        printf("Error: Failed to read back superblock from disk.\n");
        kfree(sb_verify);
        destroy(sb);
        return -1;
    }

    if (sb_verify->s_magic != sb->s_magic ||
        sb_verify->s_block_size != sb->s_block_size ||
        sb_verify->s_total_blocks != sb->s_total_blocks ||
        sb_verify->s_total_inodes != sb->s_total_inodes) {
        printf("Error: Superblock verification failed!\n");
        printf("  Expected magic=0x%X, got=0x%X\n", sb->s_magic, sb_verify->s_magic);
        printf("  Expected block_size=%u, got=%u\n", sb->s_block_size, sb_verify->s_block_size);
        printf("  Expected total_blocks=%u, got=%u\n", sb->s_total_blocks, sb_verify->s_total_blocks);
        kfree(sb_verify);
        destroy(sb);
        return -1;
    }

    printf("Superblock verification successful!\n");
    kfree(sb_verify);
    destroy(sb);

    return 0;
}