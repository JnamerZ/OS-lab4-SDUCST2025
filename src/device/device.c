#include "device/device.h"

#include "mbr/format.h"
#include "fat16-partition/format.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>

void format(Disk * disk) {
    uint8_t *temp = disk->addr;
    memset(temp, 0, DISK_SIZE);
    // init mbr

    // 63 sectors per head
    // 256 heads per cylinder 
    mbr_format((MBR *)temp);
    temp += 512;
    
    // init all partitions
    for (uint32_t i = 0; i < 4; i++) {
        PartitionRecord* ptr = (PartitionRecord *)(&(((MBR *)disk->addr)->partition_table[i]));
        partition_format((DBR *)temp, i, ptr->sector_count);
        temp += ptr->sector_count << 9; // * 512
    }
}