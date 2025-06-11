#include "fat16-directory/directory.h"

#include "utils/utils.h"
#include "utils/time16.h"
#include "main/shell.h"
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
void ls(char *args, void *shell) {
    if (args && strlen(args)) {
        printf("Too many arguments\n");
        return;
    }

    char temp[48],
         *res,
         name[16];
    
    SysDirectory *dir = &(((Shell *)shell)->dir);
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
        for (uint32_t i = 0; i < 128; i++, rec++) { // 4096 / 32 == 128
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

            snprintf(temp, 48, "%s %4d %04d-%02d-%02d %02d:%02d:%2d %s\n", 
                get_type_string(rec->type),
                rec->size,
                ((rec->date >> 9) + 80) + 1900,
                (((rec->date >> 5) & 0xf)),
                (rec->date & 0x1f),
                (rec->time >> 11),
                (((rec->time >> 5) & 0x3f)),
                (rec->time & 0x1f) << 1,
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
            FAT_corrupt();
            return;
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1) * 4096);
        clust = fat1[clust];

    } while (clust < 0xFFF8);

    printf("%s", res);
    free(res);
}

void mkdir(char *args, void *shell_addr) {
    uint32_t nameLen, extLen = 0;
    char *name, *ext;

    if (str2name_ext(args, &name, &ext, &nameLen, &extLen)) {
        return;
    }

    char nameBuf[9], extBuf[4];
    memset(nameBuf, 0, sizeof(nameBuf));
    memset(extBuf, 0, sizeof(extBuf));
    memcpy(nameBuf, name, nameLen);
    memcpy(extBuf, ext, extLen);

    Shell *shell = (Shell *)shell_addr;
    SysDirectory *dir = &(shell->dir);
    uint16_t *fat1 = dir->fat1,
             *fat2 = dir->fat2,
             clust = dir->clustNum,
             now_time, now_date;
    Record *rec = dir->content;

    do {
        for (uint32_t i = 0; i < 128; i++, rec++) {
            if ((*((uint64_t *)(rec->name)) == 0)
                && (*((uint32_t *)(rec->ext)) == 0)) {
                continue;
            }
            
            if (!strncmp(nameBuf, rec->name, 8) && !strncmp(extBuf, rec->ext, 3)) {
                printf("File or directory already exists\n");
                return;
            }
        }

        if (fat1[clust] == 0) {
            FAT_corrupt();
            return;
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1) * 4096);
        clust = fat1[clust];

    } while (clust < 0xFFF8);
    
    rec = dir->content; clust = dir->clustNum;
    uint32_t i, newClust, fatLen = dir->fatLen;
    while (1) {
        for (i = 0; i < 128; i++, rec++) {
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
        if (i < 128) {
            break;
        }

        if (fat1[clust] == 0) {
            FAT_corrupt();
            return;
        }

        if (fat1[clust] >= 0xFFF8) {
            for (i = 0; i < fatLen; i++) {
                if (!fat1[i]) {
                    // try to add a clust
                    fat1[clust] = fat2[clust] = (uint16_t)i;
                    fat1[i] = fat2[i] = 0xFFFF;
                    DirChainNode *shellBuf = shell->buf[shell->index].tail;
                    if (shellBuf) {
                        shellBuf->self->size += 4096;
                    }
                    break;
                }
            }
            if (i == fatLen) {
                printf("Partition full\n");
                return;
            }
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1) * 4096);
        clust = fat1[clust];
    }
    // add a directory
    strncpy(rec->name, name, nameLen);
    strncpy(rec->ext, ext, extLen);
    rec->type = 0x10; // dir
    get_fat16_time_date(&now_time, &now_date);
    rec->time = now_time;
    rec->date = now_date;
    rec->clustNo = (uint16_t)newClust;
    rec->size = 4096;
    fat1[newClust] = fat2[newClust] = 0xFFFF;

    rec = (Record *)(((char *)dir->content) + (newClust - dir->clustNum)*4096);
    memset(rec, 0, sizeof(Record));
    rec->name[0] = '.'; //.
    rec->type = 0x10; // dir
    rec->time = now_time;
    rec->date = now_date;
    rec->clustNo = (uint16_t)newClust;
    rec->size = 4096;
    rec += 1;
    memset(rec, 0, sizeof(Record));
    rec->name[0] = '.';
    rec->name[1] = '.'; //..
    rec->type = 0x10; // dir
    rec->time = now_time;
    rec->date = now_date;
    rec->clustNo = dir->clustNum;
    rec->size = 4096;
}

void cd(char *args, void *shell_addr) {
    uint32_t nameLen, extLen = 0;
    char *name, *ext;

    if (str2name_ext(args, &name, &ext, &nameLen, &extLen)) {
        return;
    }

    char nameBuf[9], extBuf[4];
    memset(nameBuf, 0, 9);
    memset(extBuf, 0, 4);
    strncpy(nameBuf, name, nameLen);
    strncpy(extBuf, ext, extLen);

    Shell *shell = (Shell *)shell_addr;
    SysDirectory *dir = &shell->dir;
    uint16_t *fat1 = dir->fat1,
             clust = dir->clustNum;
    Record *rec = dir->content;
    DirectoryBuffer *shellBuf = &(shell->buf[shell->index]);
    DirChainNode *p;
    do {
        for (uint32_t i = 0; i < 128; i++, rec++) {
            if (!(*((uint64_t *)(rec->name)))
                && !(*((uint32_t *)(rec->ext)))) {
                continue;
            }
            if (!strncmp(rec->name, nameBuf, 8)
                && !strncmp(rec->ext, extBuf, 3)) {    
                if (!strncmp(nameBuf, ".", 2)) {
                    return;
                }
                else if (!strncmp(nameBuf, "..", 3)) {
                    if ((p = shellBuf->tail)) {
                        if (p->prev) {
                            p = p->prev;
                            free(p->next);
                            p->next = NULL;
                            shellBuf->tail = p;
                        }
                        else {
                            free(shellBuf->tail);
                            shellBuf->head = shellBuf->tail = NULL;
                        }
                    }
                }
                else {
                    if (!shellBuf->tail) {
                        p = shellBuf->head = shellBuf->tail = calloc(1, sizeof(DirChainNode));
                        if (!p) {
                            calloc_failed();
                            exit(-1);
                        }
                        p->prev = p->next = NULL;
                    }
                    else {
                        p = shellBuf->tail;
                        p->next = calloc(1, sizeof(DirChainNode));
                        if (!p->next) {
                            calloc_failed();
                            exit(-1);
                        }
                        p->next->prev = p;
                        p = p->next;
                        p->next = NULL;
                        shellBuf->tail = p;
                    }
                    p->self = rec;
                    if (nameLen > 0 && extLen > 0) {
                        snprintf(p->name, 13, "%s.%s", nameBuf, extBuf);
                    }
                    else if (nameLen > 0) {
                        snprintf(p->name, 13, "%s", nameBuf);
                    }
                    else { //if (extLen > 0)
                        snprintf(p->name, 13, ".%s", extBuf);
                    }
                }
                dir->content = (Record *)((char *)dir->content + (rec->clustNo - dir->clustNum) * 4096);
                dir->clustNum = rec->clustNo;
                return;
            }
            
        }

        if (fat1[clust] == 0) {
            FAT_corrupt();
            return;
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1) * 4096);
        clust = fat1[clust];

    } while (clust < 0xFFF8);
    printf("No such directory: %s\n", args);
}

void delete_dir(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2) {
    uint16_t clust = rec->clustNo, temp;
    Record *p = (Record *)target;
    do {
        for (uint32_t i = 0; i < 128; i++, p++) {
            if ((*((uint64_t *)p->name) != 0x2e && (*((uint32_t *)p->ext) & 0xffffff) != 0)
                && (*((uint64_t *)p->name) != 0x2e2e && (*((uint32_t *)p->ext) & 0xffffff) != 0)
                && (*((uint64_t *)p->name) != 0 && (*((uint32_t *)p->ext) & 0xffffff) != 0)) {
                printf("Directory not empty\n");
                return;
            }
        }
        p = (Record *)((char *)p + (fat1[clust] - clust - 1)*4096);
        clust = fat1[clust];
    } while (clust < 0xFFF8);

    clust = rec->clustNo;
    do {
        if (clust == 0) {
            FAT_corrupt();
            return;
        }
        memset(p, 0, 4096);
        p = (Record *)((char *)p + (fat1[clust] - clust)*4096);
        temp = clust;
        clust = fat1[clust];
        fat1[temp] = fat2[temp] = 0;
    } while (clust < 0xFFF8);
    memset(rec, 0, sizeof(Record));
}