#include "mbr/format.h"

// 63 sectors per head
// 256 heads per cylinder 
#define GET_CHS(B, C, H, S) \
    C = B / 16128; \
    H = (B / 63) & 0xff; \
    S = ((B % 63) + 1) | ((C >> 8) & 3); \
    C &= 0xff;

void mbr_format(MBR *mbr){
    PartitionRecord *ptr;
    uint32_t block_number = 1,
             cylinder_number,
             head_number,
             sector_number,
             tmp;

    for (int i = 0; i < 4; i++) {
        ptr = &(mbr->partition_table[i]);
        GET_CHS(block_number, cylinder_number, head_number, sector_number);

        ptr->valid = '\x00';
        ptr->head_start = (uint8_t)head_number;
        ptr->sector_start = (uint8_t)sector_number;
        ptr->cylinder_start = (uint8_t)cylinder_number;

        ptr->partition_type = '\x04'; // FAT-16
        
        tmp = block_number;
        block_number += 524288; // 256MB
        block_number = (block_number > 2097151) ? 2097151 : block_number;
        GET_CHS(block_number, cylinder_number, head_number, sector_number);
        ptr->head_end = (uint8_t)head_number;
        ptr->sector_end = (uint8_t)sector_number;
        ptr->cylinder_end = (uint8_t)cylinder_number;
        ptr->sector_offset = tmp;
        ptr->sector_count = block_number - tmp;
    }
    mbr->endSig = 0xAA55; // 55 AA
}