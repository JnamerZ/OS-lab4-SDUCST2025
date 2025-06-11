#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include "fat16-directory/directory.h"

typedef struct {
} SysFile;

void delete_file(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2);
#endif