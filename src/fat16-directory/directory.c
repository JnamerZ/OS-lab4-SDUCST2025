#include "fat16-directory/directory.h"

#include "utils/utils.h"
#include "utils/time16.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

    uint16_t *fat1 = dir->fat1,
             clust = dir->clustNum;
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

        if (fat1[clust] == 0) {
            FAT_corupt();
            return;
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1) * 4096);
        clust = fat1[clust];

    } while (clust < 0xFFF8);

    printf("%s", res);
    free(res);
}

void mkdir(char *args, SysDirectory *dir) {
    uint32_t nameLen, extLen = 0;
    if (!args || !(nameLen = (uint32_t)strlen(args))) {
        printf("Missing directory name\n");
        return;
    }
    
    char *split;
    for (split = args + nameLen - 1; *split != '.' && split >= args; split--) ;

    if (split < args) {
        if (nameLen > 8) {
            printf("Too long directory name\n");
            return;
        }
        split = args + nameLen;
    }
    else {
        extLen = (uint32_t)(nameLen - (split - args) - 1);
        if (extLen > 3) {
            printf("Too long extention name\n");
            return;
        }
        nameLen = (uint32_t)(split - args);
        if (nameLen > 8) {
            printf("Too long directory name\n");
            return;
        }
        split++;
    }

    // now split == extName, args == dirName
    uint16_t *fat1 = dir->fat1,
             *fat2 = dir->fat2,
             clust = dir->clustNum,
             now_time, now_date;
    Record *rec = dir->content;
    
    uint32_t i, newClust = 0xFFFFFFFF, fatLen = dir->fatLen;
    while (1) {
        for (i = 0; i < 16; i++, rec++) {
            if (!*((uint64_t *)(rec->name))
                && !*((uint32_t *)(rec->ext))) {
                // try add a directory
                for (newClust = 0; newClust < fatLen; newClust++) {
                    if (fat1[newClust] == 0) {
                        break;
                    }
                }
                if (newClust == fatLen) {
                    printf("Partition full\n");
                    return;
                }
                break;
            }
        }
        if (i < 16) {
            break;
        }

        if (fat1[clust] == 0) {
            FAT_corupt();
            return;
        }

        if (fat1[clust] >= 0xFFF8) {
            for (i = 0; i < fatLen; i++) {
                if (!fat1[i]) {
                    // try to add a clust
                    fat1[clust] = fat2[clust] = (uint16_t)i;
                    fat1[i] = fat2[i] = 0xFFFF;
                    rec = (Record *)((char *)rec + (i - clust - 1) * 4096);

                    // try to find a new clust for new directory
                    for (newClust = 0; newClust < fatLen; newClust++) {
                        if (fat1[newClust] == 0) {
                            break;
                        }
                    }
                    if (newClust == fatLen) {
                        fat1[clust] = fat2[clust] = 0xFFFF;
                        fat1[i] = fat2[i] = 0;
                        printf("Partition full\n");
                        return;
                    }
                    break;
                }
            }
            if (i == fatLen) {
                printf("Partition full\n");
                return;
            }
            else {
                break;
            }
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1) * 4096);
        clust = fat1[clust];
    }
    // add a directory
    if (newClust == 0xFFFFFFFF) {
        printf("Mkdir failed, unknown error\n");
        return;
    }
    strncpy(rec->name, args, nameLen);
    strncpy(rec->ext, split, extLen);
    rec->type = 0x10; // dir
    get_fat16_time_date(&now_time, &now_date);
    rec->time = now_time;
    rec->date = now_date;
    rec->clustNo = (uint16_t)newClust;
    rec->size = 4096;
    fat1[newClust] = fat2[newClust] = 0xFFFF;

    rec = (Record *)((char *)rec - i*32 + (newClust - clust)*4096);
    rec->name[0] = '.'; //.
    rec->type = 0x10; // dir
    rec->time = now_time;
    rec->date = now_date;
    rec->clustNo = (uint16_t)newClust;
    rec->size = 4096;
    rec += 1;
    rec->name[0] = '.';
    rec->name[1] = '.'; //..
    rec->type = 0x10; // dir
    rec->time = now_time;
    rec->date = now_date;
    rec->clustNo = dir->clustNum;
    rec->size = 4096;
}