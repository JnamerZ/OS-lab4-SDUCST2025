#ifndef PARTITION_H
#define PARTITION_H

#include "fat16-directory/directory.h"
#include <stdint.h>

typedef struct {
    uint16_t clustEnd,
             clustCnt;
    uint16_t *fat1,
             *fat2;
    uint32_t fatLen, index;
    SysDirectory root;
} SysPartition;

void delete(char *args, void *shell);
void part(char *args, void *shell);
#endif