#include "main/init.h"

#include "mbr/mbr.h"
#include "utils/utils.h"
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>

int init(Disk *disk) {
    void *temp;

    temp = mmap(0, DISK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (temp == MAP_FAILED) {
        mmap_failed();
        return -1;
    }
    disk->addr = temp;
    format(disk);
    printf("Disk format success\nNow loading disk\n");

    load_mbr((MBR *)disk->addr, &disk->sysmbr);
    printf("Disk loaded\n");
    return 0;
}