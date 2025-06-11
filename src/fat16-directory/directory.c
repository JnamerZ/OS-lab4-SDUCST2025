#include "fat16-directory/directory.h"
#include "utils/utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// type : r&w|r-o|hid|sys|vol|dir|arc

char *get_type_string(char type){
    switch (type) {
        case 0x0:
            return "r&w";
        case 0x1:
            return "r-o";
        case 0x2:
            return "hid";
        case 0x4:
            return "sys";
        case 0x8:
            return "vol";
        case 0x10:
            return "dir";
        case 0x20:
            return "arc";
        default:
            return "unk";
    }
}

// result: *** size date time filename
// length: 3+1 + 4+1 + 10+1 + 8+1 + 12 + 1 = 42
void ls(char *args, SysDirectory * dir) {
    if (args && strlen(args)) {
        printf("Too many arguments\n");
        return;
    }

    char temp[48],
         *res,
         name[16];

    uint16_t *fat = dir->fat, clust = dir->clustNum;
    Record *rec = dir->content;
    int64_t bufferFree = 256;
    size_t bufferSize = 256, extLen;
    res =  calloc(256, sizeof(char));
    if (!res) {
        calloc_failed();
        exit(-1);
    }

    memset(temp, 0, 48);
    memset(name, 0, 16);

    do {
        for (uint32_t i = 0; i < 16; i++, rec++) { // 512 / 32 == 16
            if (!*((uint64_t *)(rec->name))) { // empty name means nofile or hidden file
                continue;
            }

            extLen = strlen(rec->ext);
            extLen = extLen > 3 ? 3 : extLen;
            
            strncpy(name, rec->name, 8);
            if (extLen) {
                strncat(name, ".", 2);
                strncat(name, rec->ext, extLen);
            }

            snprintf(temp, 48, "%s %d %04d-%02d-%02d %02d:%02d:%2d %s\n", 
                get_type_string(rec->type),
                rec->size,
                ((rec->date >> 9) + 80) + 1900,
                (((rec->date >> 5) & 0xf)),
                (rec->date & 0x1f),
                (rec->time >> 11),
                (((rec->date >> 5) & 0xf)),
                (rec->date & 0x1f) << 1,
                name
            );
            bufferFree -= (int64_t)strlen(temp);
            if (bufferFree <= 0) {
                bufferFree += 256;
                bufferSize += 256;
                res = realloc(res, bufferSize);
                if (!res) {
                    realloc_failed();
                    exit(-1);
                }
            }
            strncat(res, temp, 48);
        }

        if (fat[clust] == 0) {
            FAT_corupt();
            return;
        }
        rec = (Record *)((char *)rec + (fat[clust] - clust - 1) * 4096);
        clust = fat[clust];

    } while (clust < 0xFFF8);

    printf("%s", res);
}

void mkdir(char *args, SysDirectory *rec) {

}