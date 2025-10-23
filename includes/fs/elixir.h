#ifndef ELIXIR_H
#define ELIXIR_H

#include <stdint.h>

#define ELIXIR_SUPERBLOCK_LBA 1
#define ELIXIR_BITMAP_START_LBA 2
#define ELIXIR_MAGIC 0xE1F5

struct super_block {
    uint16_t s_magic;
    uint32_t s_free_blocks;
    uint16_t s_block_size;
    uint32_t s_total_blocks;
    uint32_t s_free_inodes;
    uint32_t s_total_inodes;
    uint32_t s_bitmap_start_lba;
    uint32_t s_bitmap_blocks;
    uint32_t s_inode_table_lba;
    uint32_t s_data_start_lba;
    uint8_t s_state;
    uint8_t s_errors;
    uint8_t padding[474];
} __attribute__((packed));

struct block_bitmap {
    uint32_t free_count;
    uint32_t used_count;
    uint32_t total;
    uint8_t *bitmap;
};

struct index {
    uint8_t time_stamp;
    uint8_t type;
    uint32_t size;
    uint32_t first_block;
    uint8_t padding[504];
} __attribute__((packed));

struct super_block* create_super(uint8_t drive);
struct block_bitmap* create_bitmap(uint8_t drive);
struct index* create_file(uint8_t drive);

int elixir_format(uint8_t drive);
int elixir_mount(uint8_t drive, struct super_block **sb_out);
int elixir_write_bitmap(uint8_t drive, struct block_bitmap *bb);
int elixir_read_bitmap(uint8_t drive, struct block_bitmap **bb_out);

#endif