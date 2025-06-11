#ifndef MBR_FORMAT_H
#define MBR_FORMAT_H

#include <stdint.h>

typedef struct {
    uint8_t valid;
    uint8_t head_start;
    uint8_t sector_start;
    uint8_t cylinder_start;
    uint8_t partition_type;
    uint8_t head_end;
    uint8_t sector_end;
    uint8_t cylinder_end;
    uint32_t sector_offset;
    uint32_t sector_count;
} __attribute__((packed)) PartitionRecord;

typedef struct {
    char bootstrap[446];
    PartitionRecord partition_table[4];
    uint16_t endSig; // 55 AA
} __attribute__((packed)) MBR;

void mbr_format(MBR *mbr);
#endif