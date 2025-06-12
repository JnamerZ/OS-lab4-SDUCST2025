#include "mbr/mbr.h"

#include "device/device.h"
#include "mbr/format.h"
#include "fat16-partition/format.h"
#include "fat16-partition/partition.h"
#include "utils/utils.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

void load_mbr(MBR *mbr, SysMBR *sysmbr) {

    for (int i = 0; i < 4; i++) {
        SysPartition *p = &(sysmbr->partitions[i]);
        PartitionRecord *pr = &(mbr->partition_table[i]);
        p->clustCnt = (uint16_t)pr->sector_count >> 9;
        p->clustEnd = p->clustCnt - 1;
        // 0 ~ cnt-1

        DBR *dbr = (DBR *)((char *)mbr + (pr->sector_offset << 9));
        p->index = dbr->volID;
        p->fat1 = (uint16_t *)((char *)dbr + 512);
        p->fat2 = (uint16_t *)((char *)(p->fat1) + (dbr->secPerFAT << 9));
        p->fatLen = (pr->sector_count - (dbr->secPerFAT << 1)) >> 3; // 8 sec per clust
        for (uint32_t j = 0; j < p->fatLen; j++) {
            if (p->fat1[j] != p->fat2[j]) {
                FAT_corrupt();
                exit(-1);
            }
        }
        p->root.content = (Record *)((char *)(p->fat2) + (dbr->secPerFAT << 9));
        p->root.content->mutex = malloc(sizeof(pthread_mutex_t));
        if (pthread_mutex_init(p->root.content->mutex, NULL)) {
            printf("Init mutex error\n");
            exit(-1);
        }
        p->root.clustNum = 0;
        p->root.fat1 = p->fat1;
        p->root.fat2 = p->fat2;
        p->root.fatLen = p->fatLen;
    }
}