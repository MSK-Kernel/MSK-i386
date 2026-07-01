#include <stdint.h>
#include "../include/ata.h"

// Forward declarations for functions from kernel.c
extern void print(const char* s);
extern void print_dec(uint32_t n);
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

// Read 16-bit word from port
static uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Wait for drive to be ready
static int ata_wait_ready(int base) {
    uint8_t status;
    int timeout = 1000000;
    
    while (timeout--) {
        status = inb(base + ATA_REG_STATUS);
        if ((status & ATA_STATUS_BSY) == 0) {
            if (status & ATA_STATUS_DRDY) {
                return 1;
            }
        }
    }
    return 0;
}

// Send IDENTIFY command to a drive
static int ata_identify(int base, int drive, uint16_t *buffer) {
    outb(base + ATA_REG_DRIVE_HEAD, 0xE0 | (drive << 4));
    
    for (int i = 0; i < 100000; i++) asm volatile ("nop");
    
    outb(base + ATA_REG_SECTOR_COUNT, 0);
    outb(base + ATA_REG_LBA_LOW, 0);
    outb(base + ATA_REG_LBA_MID, 0);
    outb(base + ATA_REG_LBA_HIGH, 0);
    
    outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    uint8_t status;
    int timeout = 1000000;
    while (timeout--) {
        status = inb(base + ATA_REG_STATUS);
        if ((status & ATA_STATUS_BSY) == 0) break;
    }
    
    if (status == 0) return 0;
    
    timeout = 1000000;
    while (timeout--) {
        status = inb(base + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR) return 0;
        if (status & 0x08) break;
    }
    
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base + ATA_REG_DATA);
    }
    
    return 1;
}

// Extract string from IDENTIFY data
static void ata_extract_string(uint16_t *buffer, int offset, int word_count, char *dest, int dest_size) {
    int idx = 0;
    char *raw = (char*)&buffer[offset];
    
    for (int i = 0; i < word_count * 2 && idx < dest_size - 1; i += 2) {
        char c1 = raw[i + 1];
        char c2 = raw[i];
        
        if (c1 != ' ') dest[idx++] = c1;
        if (c2 != ' ' && c2 != 0) dest[idx++] = c2;
    }
    dest[idx] = '\0';
    
    while (idx > 0 && dest[idx - 1] == ' ') {
        dest[idx - 1] = '\0';
        idx--;
    }
}

// Detect a single drive
int ata_detect_drive(int channel, int drive, ata_drive_info_t *info) {
    int base;
    
    if (channel == 0) {
        base = ATA_PRIMARY_IO;
    } else {
        return 0;
    }
    
    uint16_t identify[256];
    info->present = 0;
    
    if (!ata_identify(base, drive, identify)) {
        return 0;
    }
    
    ata_extract_string(identify, ATA_ID_PROD, 20, info->model, sizeof(info->model));
    ata_extract_string(identify, ATA_ID_SERIAL, 10, info->serial, sizeof(info->serial));
    ata_extract_string(identify, ATA_ID_FW_REV, 4, info->firmware, sizeof(info->firmware));
    
    info->sectors = ((uint32_t)identify[61] << 16) | identify[60];
    if (info->sectors == 0) {
        info->sectors = 0x0FFFFFFF;
    }
    
    info->present = 1;
    return 1;
}

// Initialize ATA subsystem (does nothing on startup)
void ata_init(void) {
    // ATA driver is initialized lazily - disks are detected only when
    // a command like "medialist" is run.
}
