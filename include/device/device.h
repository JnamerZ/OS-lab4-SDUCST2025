#ifndef DEVICE_H
#define DEVICE_H

#define DISK_SIZE 1073741824 // 1GB
#define SECTOR_SIZE 512 // 512B

#include <stdint.h>

#include "mbr/mbr.h"

typedef struct {
    uint8_t *addr;
    SysMBR sysmbr;
} Disk;

void format(Disk * disk);

#endif