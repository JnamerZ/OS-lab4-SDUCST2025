#include "main/shell.h"

#include "utils/utils.h"
#include "fat16-partition/partition.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

void startup(Shell *shell, Disk *disk) {
    memcpy(shell->partitions, &(disk->sysmbr.partitions), 4*sizeof(SysPartition));
    memset(shell->file, 0, 4*sizeof(SysPartition));
    for (int i = 0; i < 4; i++) {
        memcpy(&(shell->dir[i]), &(disk->sysmbr.partitions[i].root), sizeof(SysDirectory));
        shell->buf[i].head = shell->buf[i].tail = NULL;
        shell->mode[i] = 0;
    }
    shell->index = 0;
    shell->mutex = disk->mutex;
}