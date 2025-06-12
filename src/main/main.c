#include <stdio.h>
#include "device/device.h"
#include "main/init.h"
#include "main/shell.h"

Disk disk;
Shell shell;

int main(){
    setbuf(stdin, 0);
    setbuf(stdout, 0);
    if (init(&disk)) {
        printf("Init failed.\n");
        return -1;
    }
    
    startup(&shell, &disk);

    return 0;
}