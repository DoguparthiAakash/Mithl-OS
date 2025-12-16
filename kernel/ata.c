#include "ata.h"
#include "ports.h"

/* Primary Bus Ports (Standard) */
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECTOR_CNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7

void ata_init(void) {
    // Basic identification could go here, but usually just ready to use
}

static void ata_wait_bsy(void) {
    while (inb(ATA_STATUS) & ATA_STATUS_BSY);
}

static void ata_wait_drq(void) {
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ));
}

int ata_read_sector(uint32_t lba, uint8_t *buffer) {
    ata_wait_bsy();
    
    // Select Drive (Master) + LBA highest 4 bits
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_ERROR, 0); // Null byte?
    
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);
    
    // Polling wait
    uint8_t status = inb(ATA_STATUS);
    while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_DRQ) && !(status & ATA_STATUS_ERR)) {
         status = inb(ATA_STATUS);
    }
    
    // Check error
    if (status & ATA_STATUS_ERR) return 1;
    
    // Read 256 words (512 bytes)
    insw(ATA_DATA, buffer, 256);
    
    return 0;
}

int ata_write_sector(uint32_t lba, const uint8_t *buffer) {
    ata_wait_bsy();
    
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECTOR_CNT, 1);
    outb(ATA_LBA_LO, (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI, (uint8_t)(lba >> 16));
    
    outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);
    
    // Wait for RDY
    uint8_t status = inb(ATA_STATUS);
    while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_DRQ) && !(status & ATA_STATUS_ERR)) {
         status = inb(ATA_STATUS);
    }
    
    if (status & ATA_STATUS_ERR) return 1;
    
    outsw(ATA_DATA, buffer, 256);
    
    // Flush / Sync wait
    ata_wait_bsy();
    
    return 0;
}
