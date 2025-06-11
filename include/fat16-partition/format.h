#ifndef PARTITION_FORMAT_H
#define PARTITION_FORMAT_H

#include <stdint.h>
#include "fat16-directory/directory.h"

typedef struct {
    uint8_t jmp_code[3]; // EB 3C 90
    uint8_t msdos1_0[8]; // MSDOS1.7
    uint16_t bytesPerSec; // 512B
    uint8_t secPerClust; // 8
    uint16_t rsvdSecCnt; // 1
    uint8_t FAT_Number; // 2
    uint16_t rootEntCnt; // -1
    uint16_t totalSec16; // 
    uint8_t media; // 0xF0,0xF8
    uint16_t secPerFAT; // 
    uint16_t secPerTrk; // 256
    uint16_t headsNumber; // 
    uint32_t hiddSec; // 0
    uint32_t totalSec32; //
    uint8_t driverNumber; // ignore
    uint8_t reserved;
    uint8_t bootSig; // ignore
    uint32_t volID; // ignore
    uint8_t volLabel[11]; // ignore
    uint8_t fileSysType[8]; // FAT16
    uint8_t bootCode[448]; // ignore
    uint16_t bootEndSig; // 55 AA
} __attribute__((packed)) DBR;
// https://redbulb.plos-clan.org/os-tutorial/old/18-fat16-part1/

void partition_format(DBR *p, uint32_t index, uint32_t secCnt);

#endif