#ifndef MBR_H
#define MBR_H

#include <stdint.h>
#include "fat16-partition/partition.h"
#include "mbr/format.h"

typedef struct {
    SysPartition partitions[4];
} SysMBR;

void load_mbr(MBR *mbr, SysMBR *sysmbr);
#endif