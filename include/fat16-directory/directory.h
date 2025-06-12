#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <stdint.h>
#include <pthread.h>

// all bytes are null means empty string
// type: 0 - rw_file; 16 - directory
// empty name means hidden file
typedef struct {
    char name[8], ext[3],
            type;
    pthread_mutex_t * mutex;
    char reserved[2]; // use reserved store mutex_ptr
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
