#include "utils/utils.h"
#include <stdio.h>
#include <errno.h>

void calloc_failed() {
    perror("Calloc failed");
}
void realloc_failed() {
    perror("Realloc failed");
}

void mmap_failed() {
    perror("Memory map failed");
}

void FAT_corupt() {
    perror("FAT corupted");
}