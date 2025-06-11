#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>

// all bytes are null means empty string
typedef struct {
    char name[8], ext[3],
            type, reserved[10];
    uint16_t time, date, clustNo;
    uint32_t size;
}  __attribute__((packed)) Record;

typedef struct {
    uint16_t clustNum;
    uint16_t *fat1, *fat2;
    uint16_t fatLen;
    Record *content;
} SysDirectory;

void ls(char *args, SysDirectory *rec);
void mkdir(char *args, SysDirectory *rec);

#endif
