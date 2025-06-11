#include "fat16-partition/format.h"

#include <string.h>
#include "utils/time16.h"

#include <stdio.h>

void partition_format(DBR *p, uint32_t index, uint32_t secCnt) {
    // DBR init
    *(uint32_t *)&p->jmp_code = 0x00903CEB;
    *(uint64_t *)&p->msdos1_0 = 0x00372e31534f44534d;
    p->bytesPerSec = 512;
    p->secPerClust = 8;
    p->rsvdSecCnt = 1;
    p->FAT_Number = 2;
    p->rootEntCnt = 0xffff;
    p->totalSec16 = (uint16_t)((secCnt > 0xffff) ? 0 : secCnt);
    p->media = 0xF0;
    p->secPerFAT = (uint16_t)(((secCnt-1)*2%(4+512*8)) ? (secCnt-1)*2/(4+512*8) + 1 : (secCnt-1)*2/(4+512*8));
    p->secPerTrk = 256;
    p->headsNumber = (uint16_t)(secCnt >> 8);
    p->hiddSec = 0;
    p->totalSec32 = (secCnt > 0xffff) ? secCnt : 0;
    p->volID = index;
    p->volLabel[0] = (uint8_t)index;
    *(uint64_t *)&p->fileSysType = 0x003631544146;
    p->bootEndSig = 0xAA55;

    // fat init
    uint16_t *fat1, *fat2;
    uint32_t temp;
    temp = p->secPerFAT << 9;
    fat1 = (uint16_t *)(((char *)p) + 512);
    fat2 = (uint16_t *)(((char *)fat1) + temp);
    memset(fat1, 0, temp);
    memset(fat2, 0, temp);

    // root directory init
    Record* root = (Record *)(((char *)fat2) + temp);
    uint16_t now_time, now_date;
    get_fat16_time_date(&now_time, &now_date);

    root->name[0] = '.'; //.
    root->type = 0x10; // dir
    root->time = now_time;
    root->date = now_date;
    root->clustNo = 0;
    root->size = 4096; // bytesPerClust
    root += 1;
    root->name[0] = '.';
    root->name[1] = '.'; //..
    root->type = 0x10; // dir
    root->time = now_time;
    root->date = now_date;
    root->clustNo = 0;
    root->size = 4096; // bytesPerClust
    fat1[0] = fat2[0] = 0xFFFF;
}