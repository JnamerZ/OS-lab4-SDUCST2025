#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include "fat16-directory/directory.h"

typedef struct {
    uint16_t clustNum;
    uint32_t mode, size, read, write;
    char *content;
} SysFile;

void delete_file(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2);
void open(char *args, void *shell);
void close(char *args, void *shell);
#endif