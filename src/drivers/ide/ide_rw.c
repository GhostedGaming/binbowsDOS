#include <ide.h>
#include <commands.h>
#include <timer.h>

#define SECTOR_SIZE_BYTES 512

/* ============================================================================
 * SECTOR READ
 * ============================================================================ */

int ide_read_sectors(uint8_t drive, uint8_t numsects, uint32_t lba, void *buf) {
    if (drive > 3 || !ide_devices[drive].Reserved)
        return 1;

    uint8_t channel = ide_devices[drive].Channel;
    uint8_t slavebit = ide_devices[drive].Drive;
    uint16_t bus = channels[channel].base;
    uint8_t lba_mode = 0;

    uint8_t lba_io[6];
    uint8_t head;

    uint16_t i;
    uint8_t err;

    if (lba >= 0x10000000) {
        lba_mode = 2;
        lba_io[0] = (lba & 0x000000FF) >> 0;
        lba_io[1] = (lba & 0x0000FF00) >> 8;
        lba_io[2] = (lba & 0x00FF0000) >> 16;
        lba_io[3] = (lba & 0xFF000000) >> 24;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = 0;
    } else {
        lba_mode = 1;
        lba_io[0] = (lba >> 0) & 0xFF;
        lba_io[1] = (lba >> 8) & 0xFF;
        lba_io[2] = (lba >> 16) & 0xFF;
        lba_io[3] = (lba >> 24) & 0x0F;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba >> 24) & 0x0F;
    }

    ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);
    timer_wait_ms(1);

    ide_write(channel, ATA_REG_SECCOUNT0, numsects);
    ide_write(channel, ATA_REG_LBA0, lba_io[0]);
    ide_write(channel, ATA_REG_LBA1, lba_io[1]);
    ide_write(channel, ATA_REG_LBA2, lba_io[2]);
    ide_write(channel, ATA_REG_COMMAND, ATA_CMD_READ_PIO);

    for (i = 0; i < numsects; i++) {
        if ((err = ide_polling(channel, 1))) return err;
        insw(bus, (uint16_t *)((uint8_t *)buf + i * 512), 256);
    }

    (void)lba_mode;

    return 0;
}

/* ============================================================================
 * SECTOR WRITE
 * ============================================================================ */

int ide_write_sectors_counted(uint8_t drive, uint32_t start_lba, size_t byte_count, const void *buf) {
    if (drive > 3 || !ide_devices[drive].Reserved)
        return 1;

    if (byte_count == 0)
        return 0;

    if (byte_count % 512)
        return 2;

    size_t total_sectors = byte_count / 512;
    const uint8_t *data = (const uint8_t *)buf;
    size_t sectors_written = 0;
    size_t sectors_remaining = total_sectors;
    uint32_t lba = start_lba;
    int ret;

    const uint32_t LBA28_MAX = 0x0FFFFFFF;

    while (sectors_remaining) {
        if (lba > LBA28_MAX) return 3;

        uint8_t n = (sectors_remaining >= 256) ? 0 : (uint8_t)sectors_remaining;

        uint8_t channel = ide_devices[drive].Channel;
        uint8_t slavebit = ide_devices[drive].Drive;
        uint16_t bus = channels[channel].base;
        uint8_t lba_io[6];
        uint8_t head;

        lba_io[0] = (lba >> 0) & 0xFF;
        lba_io[1] = (lba >> 8) & 0xFF;
        lba_io[2] = (lba >> 16) & 0xFF;
        lba_io[3] = (lba >> 24) & 0x0F;
        lba_io[4] = 0;
        lba_io[5] = 0;
        head = (lba >> 24) & 0x0F;

        ide_write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head);
        timer_wait_ms(1);

        ide_write(channel, ATA_REG_SECCOUNT0, n);
        ide_write(channel, ATA_REG_LBA0, lba_io[0]);
        ide_write(channel, ATA_REG_LBA1, lba_io[1]);
        ide_write(channel, ATA_REG_LBA2, lba_io[2]);
        ide_write(channel, ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

        uint16_t sectors_to_transfer = (n == 0) ? 256 : n;
        for (uint16_t i = 0; i < sectors_to_transfer; i++) {
            uint8_t err;
            if ((err = ide_polling(channel, 0))) return err;
            outsw(bus, (const uint16_t *)(data + (sectors_written + i) * 512), 256);
        }

        ide_write(channel, ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
        if ((ret = ide_polling(channel, 0))) return ret;

        sectors_written += sectors_to_transfer;
        sectors_remaining -= sectors_to_transfer;
        lba += sectors_to_transfer;
    }

    return 0;
}

uint32_t find_next_free_lba(uint8_t drive) {
    extern struct ide_device ide_devices[4];

    if (drive >= 4) return UINT32_MAX;
    if (!ide_devices[drive].Reserved) return UINT32_MAX;

    uint32_t size = ide_devices[drive].Size;
    if (size == 0) return UINT32_MAX;

    uint8_t buf[512];
    int seen_used = 0;
    uint32_t last_used_lba = 0xFFFFFFFF;

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

    if (!seen_used) return 0;            /* no used sectors, next free is LBA 0 */
    if (last_used_lba == size - 1) return UINT32_MAX; /* no free space after last used */
    return last_used_lba + 1;            /* start sector of the next free */
}