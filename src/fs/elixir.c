#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <mem.h>
#include <ide.h>
#include <fs/elixir.h>

/**
 * find_next_free_lba - find first free LBA after the last used sector
 * @drive:  drive index (0..3) into ide_devices
 *
 * Scans the disk from LBA 0..Size-1. A sector is considered "used" if any
 * byte in the 512-byte sector != 0. Returns:
 *  - 0 if the disk is completely free,
 *  - last_used_lba + 1 for the first free LBA after the final used sector,
 *  - UINT32_MAX on error or if there is no free LBA after the last used sector.
 */
uint32_t find_next_free_lba(uint8_t drive) {
    extern struct ide_device ide_devices[4];

    if (drive >= 4) return UINT32_MAX;
    if (!ide_devices[drive].Reserved) return UINT32_MAX;

    uint64_t size64 = ide_devices[drive].Size;
    if (size64 == 0 || size64 > UINT32_MAX) return UINT32_MAX;
    uint32_t size = (uint32_t)size64;

    uint8_t buf[512];
    int seen_used = 0;
    uint32_t last_used_lba = 0;

    for (uint32_t lba = 0; lba < size; ++lba) {
        if (ide_read_sectors(drive, 1, lba, buf) != 0) {
            return UINT32_MAX;
        }

        int all_zero = 1;
        for (size_t i = 0; i < sizeof(buf); ++i) {
            if (buf[i] != 0) {
                all_zero = 0;
                break;
            }
        }

        if (!all_zero) {
            seen_used = 1;
            last_used_lba = lba;
        }
    }

    if (!seen_used) return 0;
    if (last_used_lba == size - 1) return UINT32_MAX;
    return last_used_lba + 1;
}

/**
 * elixir_format - write Elixir superblock at the first available LBA
 * @drive: drive index (0..3)
 *
 * Uses find_next_free_lba() to determine where to place the superblock.
 * Writes the in-memory super_block created by create_super() using
 * ide_write_sectors_counted(). Returns 0 on success, -1 on failure.
 */
int elixir_format(uint8_t drive) {
    struct super_block *sb = create_super(drive);
    if (!sb) {
        printf("Error: create_super failed\n");
        return -1;
    }

    uint32_t next_lba = find_next_free_lba(drive);
    if (next_lba == UINT32_MAX) {
        printf("Error: cannot determine next free LBA for drive %u\n", (unsigned)drive);
        destroy(sb);
        return -1;
    }

    size_t sb_bytes = sizeof(struct super_block);
    /* ide_write_sectors_counted expects byte count and start LBA */
    if (ide_write_sectors_counted(drive, next_lba, sb_bytes, (const void *)sb) != 0) {
        printf("Error: failed to write superblock to drive %u at LBA %u\n",
               (unsigned)drive, (unsigned)next_lba);
        destroy(sb);
        return -1;
    }

    /* Optional verification read */
    void *verify_buf = kmalloc(sb_bytes);
    if (!verify_buf) {
        printf("Warning: verification allocation failed; format completed\n");
        destroy(sb);
        return 0;
    }

    if (ide_read_sectors(drive, (sb_bytes + 511) / 512, next_lba, verify_buf) != 0) {
        printf("Warning: failed to read back superblock for verification\n");
        kfree(verify_buf);
        destroy(sb);
        return 0;
    }

    if (memcmp(sb, verify_buf, sb_bytes) != 0) {
        printf("Error: superblock verification failed\n");
        kfree(verify_buf);
        destroy(sb);
        return -1;
    }

    kfree(verify_buf);
    destroy(sb);
    return 0;
}