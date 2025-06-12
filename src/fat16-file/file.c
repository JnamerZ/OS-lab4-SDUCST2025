#include "fat16-file/file.h"

#include "utils/utils.h"
#include "main/shell.h"
#include "utils/time16.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MIN(X, Y) ((X < Y) ? X : Y)
#define MAX(X, Y) ((X > Y) ? X : Y)

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
                if (rec->type == 0x10) {
                    return 2;
                }
                else {
                    *res = rec;
                    return 0;
                }
                
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
    fat1[freeClust[0]] = fat2[freeClust[0]] = 0xFFFF;
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
    int result = find_file(dir, nameBuf, extBuf, &rec);
    if (result == 1 && mode == 0) {
        printf("No such file: %s\n", args);
        return;
    }
    if (result == 2) {
        printf("A directory with given file name already exists\n");
        return;
    }
    DirectoryBuffer *shellBuf = &(shell->buf[shell->index]);
    DirChainNode *p; SysFile *file = &shell->file;
    uint16_t *fat1 = shell->partition->fat1;
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
            p->mode = 1;

            file->clustNum = rec->clustNo;
            file->mode = 0;
            file->size = rec->size;
            file->alreadyRead = 0;
            file->alreadyWrite = 0;
            file->remainWrite = 0;
            file->remainRead = 4096;
            file->write = file->read = file->content = ((char *)dir->content + (rec->clustNo - dir->clustNum)*4096);
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
            p->mode = 1;
            file->clustNum = rec->clustNo;
            file->mode = mode;
            file->size = rec->size;
            file->alreadyRead = 0;
            file->remainRead = 4096;
            file->read = file->content = ((char *)dir->content + (rec->clustNo - dir->clustNum)*4096);
            if (mode == 2) {
                uint32_t clust_offset = rec->size / 4096,
                         clust = rec->clustNo;
                for (uint32_t i = 0; i < clust_offset; i++) {
                    clust = fat1[clust];
                }
                if (clust == 0 || clust >= 0xFFF8) {
                    FAT_corrupt();
                    exit(-1);
                }
                file->write = ((char*)dir->content + (clust - dir->clustNum)*4096 + (rec->size%4096));
                file->alreadyWrite = rec->size;
                file->remainWrite = 4096 - (rec->size % 4096);
            }
            else {
                file->write = file->read;
                file->alreadyWrite = 0;
                file->remainWrite = 4096;
            }
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
        shellBuf->tail = p;
    }
    else {
        free(p);
        shellBuf->head = shellBuf->tail = NULL;
    }
}

void read(char *args, void *shell_addr) {
    Shell *shell = (Shell *)shell_addr;
    if (!shell->mode) {
        printf("Open a file first\n");
        return;
    }

    uint32_t size = (uint32_t)atoi(args);
    SysFile *file = &shell->file;

    if (size == 0xFFFFFFFF) { // -1
        file->read = file->content;
        file->alreadyRead = 0;
        file->remainRead = MIN(file->size, 4096);
        return;
    }

    uint32_t realSize = MIN(file->size - file->alreadyRead, size);

    char *buffer = calloc(realSize, sizeof(char));
    if (!buffer) {
        calloc_failed();
        exit(-1);
    }

    size = realSize;
    
    uint16_t clust = file->clustNum,
             clust_offset = (uint16_t)(file->alreadyRead / 4096),
             *fat1 = shell->partition->fat1;
    for (uint16_t i = 0; i < clust_offset; i++) {
        clust = fat1[clust];
    }
    while (size > file->remainRead) {
        strncat(buffer, file->read, file->remainRead);
        file->alreadyRead += file->remainRead;
        size -= file->remainRead;
        clust = fat1[clust];
        file->read = (file->content + (clust - file->clustNum)*4096);
        file->remainRead = MIN(file->size - file->alreadyRead, 4096);
    }
    strncat(buffer, file->read, size);
    file->read += size;
    file->alreadyRead += size;
    file->remainRead -= size;
    
    printf("Read %u bytes:\n", realSize);
    uint32_t temp = realSize / 16;
    char buf;
    for (uint32_t i = 0; i < temp; i++) {
        printf("%u | ", i);
        for (uint32_t j = 0; j < 16; j++) {
            printf("%02X ", buffer[i * 16 + j]);
        }
        printf("| ");
        for (uint32_t j = 0; j < 16; j++) {
            buf = buffer[i * 16 + j];
            if (buf < 127 && buf > 31) {
                printf("%c", buf);
            }
            else {
                printf("%c", '.');
            }
        }
        printf(" |\n");
    }
    if (realSize & 0xf) {
        printf("%u | ", temp);
        temp *= 16;
        for (uint32_t i = temp; i < realSize; i++) {
            printf("%02X ", buffer[i]);
        }
        printf("| ");
        for (uint32_t i = temp; i < realSize; i++) {
            buf = buffer[i];
            if (buf < 127 && buf > 31) {
                printf("%c", buf);
            }
            else {
                printf("%c", '.');
            }
        }
        printf(" |\n");
    }
    free(buffer);
}

void write(char *args, void *shell_addr) {
    Shell *shell = (Shell *)shell_addr;
    if (!shell->mode) {
        printf("Open a file first\n");
        return;
    }
    SysFile *file = &shell->file;
    if (file->mode == 0) {
        printf("Read only mode\n");
        return;
    }
    
    uint32_t size = (uint32_t)atoi(args);

    if (size == 0xFFFFFFFF) { // -1
        file->write = file->content;
        file->alreadyWrite = 0;
        file->remainWrite = MIN(file->size, 4096);
        return;
    }

    uint16_t clust = file->clustNum,
             clust_offset = (uint16_t)(file->alreadyWrite / 4096),
             *fat1 = shell->partition->fat1,
             *fat2 = shell->partition->fat2;
    uint32_t fatLen = shell->partition->fatLen;

    uint32_t newSize = file->alreadyWrite + size,
             newClustCnt = (newSize / 4096) - (file->size / 4096);
    uint16_t *newClusts, cnt = 0;

    for (uint16_t i = 0; i < clust_offset; i++) {
        clust = fat1[clust];
        if (clust == 0 || clust >= 0xFFF8) {
            FAT_corrupt();
            exit(-1);
        }
    }

    if (newSize > file->size) {
        newClusts = calloc(newClustCnt, sizeof(uint16_t));
        if (!newClusts) {
            calloc_failed();
            exit(-1);
        }
        for (uint32_t i = 0; i < fatLen && cnt < newClustCnt; i++) {
            if (!fat1[i]) {
                newClusts[cnt++] = (uint16_t)i;
            }
        }
        if (cnt < newClustCnt) {
            printf("Partition full\n");
            free(newClusts);
            return;
        }
        cnt = clust;
        for (uint32_t i = 0; i < newClustCnt; i++) {
            fat1[cnt] = fat2[cnt] = newClusts[i];
            cnt = newClusts[i];
        }
        fat1[cnt] = fat2[cnt] = 0xFFFF;
        file->size = newSize;
        free(newClusts);
    }

    char *buffer = calloc(size + 1, sizeof(char));
    if (!buffer) {
        calloc_failed();
        exit(-1);
    }

    printf("Writing %u bytes:\n> ", size);
    fgets(buffer, (int)size + 1, stdin);
    
    while (size > file->remainWrite) {
        memcpy(file->write, buffer, file->remainWrite);
        file->alreadyWrite += file->remainWrite;
        size -= file->remainWrite;
        clust = fat1[clust];
        file->write = (file->content + (clust - file->clustNum)*4096);
        file->remainWrite = MIN(file->size - file->alreadyWrite, 4096);
    }
    memcpy(file->write, buffer, size);
    file->write += size;
    file->alreadyWrite += size;
    file->remainWrite -= size;

    printf("Writed %u bytes\n", size);
    
    shell->buf[shell->index].tail->self->size = file->size;
    free(buffer);
}
