#include "fat16-file/file.h"

#include "utils/utils.h"
#include "main/shell.h"
#include "utils/time16.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void delete_file(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2) {
    uint16_t clust = rec->clustNo, temp;
    do {
        if (clust == 0) {
            FAT_corrupt();
            return;
        }
        memset(target, 0, 4096);
        temp = clust;
        clust = fat1[clust];
        fat1[temp] = fat2[temp] = 0;
        target = (target + (clust - temp)*4096);
    } while (clust < 0xFFF8);
    memset(rec, 0, sizeof(Record));
}

int find_file(SysDirectory *dir, char* name, char* ext, Record **res) {
    Record *empty = NULL, *rec = dir->content;
    uint16_t clust = dir->clustNum,
             *fat1 = dir->fat1;
    do {
        for (uint32_t i = 0; i < 128; i++, rec++) {
            if ((*((uint64_t *)(rec->name)) == 0)
                && (*((uint32_t *)(rec->ext)) == 0)) {
                if (!empty) {
                    empty = rec;
                }
                continue;
            }
            
            if (!strncmp(name, rec->name, 8) && !strncmp(ext, rec->ext, 3)) {
                *res = rec;
                return 0;
            }
        }
        rec = (Record *)((char *)rec + (fat1[clust] - clust - 1)*4096);
        clust = fat1[clust];
    } while (clust < 0xFFF8);

    *res = empty;
    return 1;
}

int create_file(SysDirectory *dir, Record **res) {
    uint16_t freeClust[2] = {0, 0}, cnt = 0,
             *fat1 = dir->fat1,
             *fat2 = dir->fat2,
             now_time, now_date;
    uint32_t clust, fatLen = dir->fatLen;

    Record *rec = *res;
    if (rec && ((*((uint64_t *)(rec->name)) != 0) || (*((uint32_t *)((*res)->ext)) != 0))) {
        return 0;
    }
    else if (!rec) {
        for (clust = 0; clust < fatLen; clust++) {
            if (!fat1[clust]) {
                freeClust[cnt++] = (uint16_t)clust;
                break;
            }
        }
        if (!cnt) {
            goto CREATE_FILE_FAIL;
        }
    }
    
    for (clust = (cnt ? freeClust[0]+1 : 0);
         clust < fatLen; clust++) {
        
        if (!fat1[clust]) {
            freeClust[cnt++] = (uint16_t)clust;
            break;
        }
    }

    if (cnt < 1 + (!rec ? 1 : 0)) {
CREATE_FILE_FAIL:
        printf("Partition full\n");
        return 1;
    }

    if (!rec) {
        for (clust = dir->clustNum; fat1[clust] < 0xFFF8; clust = fat1[clust]) ;
        fat1[clust] = fat2[clust] = freeClust[1];
        fat1[freeClust[1]] = fat2[freeClust[1]] = 0xFFFF;
        rec = (Record *)((char *)dir->content + (freeClust[1] - dir->clustNum)*4096);
    }
    rec->clustNo = freeClust[0];
    rec->size = 0;

    get_fat16_time_date(&now_time, &now_date);
    rec->time = now_time; rec->date = now_date;
    *res = rec;
    return 0;
}

// modes:
// read-only   - 0
// read-write  - 1
// read-write+ - 2 (write-append)
// create file when *write* but file not found

void open(char *args, void *shell_addr) {
    Shell *shell = (Shell *)shell_addr;
    if (shell->mode) {
        printf("Already opening a file\n");
        return;
    }
    uint32_t nameLen, extLen = 0;
    char *name, *ext;

    char *nextArg = strstr(args, " ");
    if (!nextArg) {
        printf("Missing argument: mode\n");
        return;
    }
    nextArg[0] = 0; nextArg++;
    if (str2name_ext(args, &name, &ext, &nameLen, &extLen)) {
        return;
    }

    char nameBuf[9], extBuf[4];
    memset(nameBuf, 0, sizeof(nameBuf));
    memset(extBuf, 0, sizeof(extBuf));
    memcpy(nameBuf, name, nameLen);
    memcpy(extBuf, ext, extLen);

    uint32_t mode;
    mode = (uint32_t)atoi(nextArg);

    SysDirectory *dir = &(shell->dir);
    Record *rec = NULL;
    if (find_file(dir, nameBuf, extBuf, &rec) && mode == 0) {
        printf("No such file\n");
        return;
    }
    DirectoryBuffer *shellBuf = &(shell->buf[shell->index]);
    DirChainNode *p; SysFile *file = &shell->file;
    switch (mode) {
        case 0:
            shell->mode = 1;
            if (shellBuf->tail) {
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
            else {
                p = shellBuf->head = shellBuf->tail = calloc(1, sizeof(DirChainNode));
                if (!p) {
                    calloc_failed();
                    exit(-1);
                }
                p->prev = p->next = NULL;
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

            file->clustNum = rec->clustNo;
            file->mode = 0;
            file->size = rec->size;
            file->write = file->read = 0;
            file->content = ((char *)dir->content + (rec->clustNo - dir->clustNum)*4096);
            break;
        case 1:
        case 2:
            if (create_file(dir, &rec) || !rec) {
                return;
            }
            memcpy(rec->name, nameBuf, 8);
            memcpy(rec->ext, extBuf, 3);
            rec->type = 0;

            shell->mode = 1;
            if (shellBuf->tail) {
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
            else {
                p = shellBuf->head = shellBuf->tail = calloc(1, sizeof(DirChainNode));
                if (!p) {
                    calloc_failed();
                    exit(-1);
                }
                p->prev = p->next = NULL;
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
            
            file->clustNum = rec->clustNo;
            file->mode = mode;
            file->size = rec->size;
            file->read = 0;
            file->write = mode == 1 ? 0 : rec->size;
            file->content = ((char *)dir->content + (rec->clustNo - dir->clustNum)*4096);
            break;
        default:
            printf("Invalid mode\n");
            return;
    }
    
}

void close(char *args, void *shell_addr) {
    if (args && strlen(args)) {
        printf("Too many arguments\n");
        return;
    }
    
    Shell *shell = (Shell *)shell_addr;
    if (!shell->mode) {
        printf("Not opening a file\n");
        return;
    }

    shell->mode = 0;
    DirectoryBuffer *shellBuf = &shell->buf[shell->index];
    DirChainNode *p = shellBuf->tail;
    if (!p) {
        printf("Broken dir buffer\n");
        exit(-1);
    }

    if (p->prev) {
        p = p->prev;
        free(p->next);
        p->next = NULL;
    }
    else {
        free(p);
        shellBuf->head = shellBuf->tail = NULL;
    }
}
