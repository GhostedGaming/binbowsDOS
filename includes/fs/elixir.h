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

struct inode {
    uint32_t i_mode;        // File mode
    uint32_t i_uid;         // Owner UID
    uint32_t i_size;        // Size in bytes
    uint32_t i_atime;       // Access time
    uint32_t i_ctime;       // Creation time
    uint32_t i_mtime;       // Modification time
    uint32_t i_dtime;       // Deletion time
    uint16_t i_gid;         // Group ID
    uint16_t i_links_count; // Links count
    uint32_t i_blocks;      // Number of blocks allocated
    uint32_t i_block[15];   // Pointers to blocks
};

struct super_block* create_super(uint8_t drive);
int elixir_format(uint8_t drive);

#endif