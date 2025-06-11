#include "fat16-partition/partition.h"

#include "main/shell.h"
#include "utils/utils.h"
#include "fat16-directory/directory.h"
#include "fat16-file/file.h"
#include <string.h>
#include <stdio.h>

void delete(char *args, void *shell_addr) {
    uint32_t nameLen, extLen = 0;
    char *name, *ext, *target;

    if (str2name_ext(args, &name, &ext, &nameLen, &extLen)) {
        return;
    }

    char nameBuf[9], extBuf[4];
    memset(nameBuf, 0, 9);
    memset(extBuf, 0, 4);
    strncpy(nameBuf, name, nameLen);
    strncpy(extBuf, ext, extLen);

    if ((*((uint64_t *)nameBuf) == 0x2e && (*((uint32_t *)extBuf) & 0xffffff) == 0)
        || (*((uint64_t *)nameBuf) == 0x2e2e && (*((uint32_t *)extBuf) & 0xffffff) == 0)) {
        printf("Invalid file name or directory name: %s\n", args);
        return;
    }

    Shell *shell = (Shell *)shell_addr;
    SysDirectory *dir = &shell->dir;
    uint16_t *fat1 = dir->fat1,
             *fat2 = dir->fat2,
             clust = dir->clustNum;
    Record *rec = dir->content;
    do {
        rec += 2;
        for (uint32_t i = 0; i < 128; i++, rec++) {
            if ((*((uint64_t *)(rec->name)) == 0)
                && (*((uint32_t *)(rec->ext)) == 0)) {
                continue;
            }
            
            if (!strncmp(nameBuf, rec->name, 8) && !strncmp(extBuf, rec->ext, 3)) {
                target = ((char *)dir->content + (rec->clustNo - clust)*4096);
                if (rec->type == 0x10) { // dir
                    delete_dir(rec, target, fat1, fat2);
                }
                else {
                    if (shell->mode == 0) {
                        delete_file(rec, target, fat1, fat2);
                    }
                    else {
                        printf("Opening a file, please close it first\n");
                    }
                }
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
    printf("No such file or directory: %s\n", args);
}