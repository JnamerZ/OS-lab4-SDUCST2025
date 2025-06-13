#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include "fat16-directory/directory.h"

typedef struct {
    uint16_t clustNum;
    uint32_t mode, size,
             alreadyRead, alreadyWrite,
             remainRead, remainWrite;
    char *content, *read, *write;
} SysFile;

void delete_file(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2);
void f_open(char *args, void *shell);
void f_close(char *args, void *shell);
void f_read(char *args, void *shell);
void f_write(char *args, void *shell);
#endif