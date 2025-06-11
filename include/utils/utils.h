#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

void calloc_failed();
void realloc_failed();
void mmap_failed();
void FAT_corrupt();
int str2name_ext(char *input, char **name, char **ext, uint32_t *nameLen, uint32_t *extLen);

#endif