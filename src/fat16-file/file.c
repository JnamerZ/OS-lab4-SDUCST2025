#include "fat16-file/file.h"

#include "utils/utils.h"
#include <string.h>
#include <stdlib.h>

void delete_file(Record *rec, char *target, uint16_t *fat1, uint16_t *fat2) {
    uint16_t clust = rec->clustNo, temp;
    do {
        if (clust == 0) {
            FAT_corupt();
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
