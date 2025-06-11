#include "utils/utils.h"
#include <stdio.h>
#include <string.h>
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
    printf("FAT corupted\n");
}

int str2name_ext(char *input, char **name, char **ext, uint32_t *nameLen, uint32_t *extLen) {
    *extLen = 0;
    if (!input || !(*nameLen = (uint32_t)strlen(input))) {
        printf("Missing directory name\n");
        return -1;
    }

    if (!strncmp(input, ".", 2)) {
        *name = input;
        *ext = input + 1;
        *nameLen = 1;
        *extLen = 0;
        return 0;
    }

    if (!strncmp(input, "..", 3)) {
        *name = input;
        *ext = input + 2;
        *nameLen = 2;
        *extLen = 0;
        return 0;
    }

    char *split;
    
    for (split = input + *nameLen - 1; *split != '.' && split >= input; split--) ;

    if (split < input) {
        if (*nameLen > 8) {
            printf("Too long directory name\n");
            return -1;
        }
        split = input + *nameLen;
    }
    else {
        *extLen = (uint32_t)(*nameLen - (split - input) - 1);
        if (*extLen > 3) {
            printf("Too long splitention name\n");
            return -1;
        }
        *nameLen = (uint32_t)(split - input);
        if (*nameLen > 8) {
            printf("Too long directory name\n");
            return -1;
        }
        split++;
    }
    *name = input;
    *ext = split;
    return 0;
}
