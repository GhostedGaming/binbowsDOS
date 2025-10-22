#ifndef ELIXIR_H
#define ELIXIR_H

#include <stdint.h>

struct super_block {
    // Magic number to identify the filesystem
    uint16_t s_magic;
    // Block information
    uint32_t s_free_blocks;
    uint16_t s_block_size;
    uint32_t s_total_blocks;
    // Inode information
    uint32_t s_free_inodes;
    uint32_t s_total_inodes;
    // Filesystem state
    uint8_t s_state;
    uint8_t s_errors;
    // Padding to ensure the struct is exactly 512 bytes 
    uint8_t  padding[480];
};

struct block_bitmap {
    uint32_t free_count;
    uint32_t used_count;
    uint32_t total;
}

struct index {
    uint8_t time_stamp;
    uint8_t type;
    // uint8_t permissions
    uint32_t size;
}

struct super_block* create_super(uint8_t drive);
int elixir_format(uint8_t drive);

#endif