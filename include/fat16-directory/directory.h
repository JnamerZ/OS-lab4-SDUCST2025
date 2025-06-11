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
    uint32_t fatLen;
    uint16_t *fat1, *fat2;
    Record *content;
} SysDirectory;

void ls(char *args, void *shell);
void mkdir(char *args, void *shell);
void cd(char *args, void *shell);
void delete_dir(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2);
#endif
