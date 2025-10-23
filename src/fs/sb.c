#include <fs/elixir.h>
#include <vga.h>
#include <mem.h>
#include <ide.h>

struct super_block* create_super(uint8_t drive) {
    extern struct ide_device ide_devices[4];
    struct super_block* sb;
    uint16_t sector_size = 512;
    uint32_t total_sectors;
    uint16_t sectors_per_block;
    uint32_t bitmap_size_sectors;

    if (drive >= 4) {
        printf("Error: Invalid drive index %d.\n", drive);
        return NULL;
    }

    total_sectors = ide_devices[drive].Size;

    printf("Initializing Elixir filesystem on drive %d...\n", drive);
    printf("Drive size: %u sectors (%u MB)\n",
           total_sectors, total_sectors / 2048);

    printf("Allocating super_block struct...\n");
    sb = kmalloc(sizeof(struct super_block));
    if (!sb) {
        printf("Failed to allocate super_block!\n");
        return NULL;
    }

    memset(sb, 0, sizeof(struct super_block));

    if (total_sectors < 32768)               return NULL;
    else if (total_sectors < 131072)         sectors_per_block = 2;
    else if (total_sectors < 524288)         sectors_per_block = 4;
    else if (total_sectors < 2097152)        sectors_per_block = 8;
    else if (total_sectors < 8388608)        sectors_per_block = 16;
    else if (total_sectors < 33554432)       sectors_per_block = 32;
    else if (total_sectors < 134217728)      sectors_per_block = 64;
    else                                     sectors_per_block = 128;

    sb->s_magic = ELIXIR_MAGIC;
    sb->s_block_size = sector_size * sectors_per_block;
    sb->s_total_blocks = total_sectors / sectors_per_block;
    
    bitmap_size_sectors = (total_sectors + 4095) / 4096;
    if (bitmap_size_sectors < 1) bitmap_size_sectors = 1;
    
    sb->s_bitmap_start_lba = ELIXIR_BITMAP_START_LBA;
    sb->s_bitmap_blocks = (bitmap_size_sectors + sectors_per_block - 1) / sectors_per_block;
    sb->s_inode_table_lba = sb->s_bitmap_start_lba + (sb->s_bitmap_blocks * sectors_per_block);
    sb->s_data_start_lba = sb->s_inode_table_lba + 256;
    
    sb->s_free_blocks = sb->s_total_blocks;
    sb->s_total_inodes = 128;
    sb->s_free_inodes = 128;
    sb->s_state = 1;
    sb->s_errors = 0;

    printf("Superblock initialized:\n");
    printf("  magic=0x%X\n", sb->s_magic);
    printf("  block_size=%u bytes\n", sb->s_block_size);
    printf("  total_blocks=%u\n", sb->s_total_blocks);
    printf("  total_inodes=%u\n", sb->s_total_inodes);
    printf("  bitmap_start=%u\n", sb->s_bitmap_start_lba);
    printf("  data_start=%u\n", sb->s_data_start_lba);

    return sb;
}