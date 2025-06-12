#ifndef SHELL_H
#define SHELL_H

#include "device/device.h"
#include "fat16-partition/partition.h"
#include "fat16-directory/directory.h"
#include "fat16-file/file.h"
#include <stdint.h>

typedef struct DIR_BUF_CHAIN {
    struct DIR_BUF_CHAIN *prev, *next;
    char name[13]; Record *self;
    uint8_t mode;
} DirChainNode;

typedef struct {
    DirChainNode *head, *tail;
} DirectoryBuffer;

typedef struct {
    SysPartition *partition;
    uint8_t index, mode;
    SysDirectory dir;
    SysFile file;
    DirectoryBuffer buf[4];
} Shell;

void startup(Shell *shell, Disk *disk);

#endif