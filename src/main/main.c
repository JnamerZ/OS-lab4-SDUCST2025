#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "utils/utils.h"
#include "device/device.h"
#include "main/init.h"
#include "main/shell.h"

Disk disk;
pthread_mutex_t disk_mutex = PTHREAD_MUTEX_INITIALIZER;
Shell shell[3];

void *thread1_func(void *) {
    char arg[256];
    strncpy(arg, "a 1", 4);
    f_open(arg, (void *)&shell[0]);
    strncpy(arg, "4", 2);
    f_write(arg, (void *)&shell[0]);
    f_close(NULL, (void *)&shell[0]);
    for (int i = 0; i < 12; i++) {
        usleep(5);
        pthread_mutex_lock(&disk_mutex);
        printf("Appending new bytes %d\n", i);
        pthread_mutex_unlock(&disk_mutex);
        strncpy(arg, "a 2", 4);
        f_open(arg, (void *)&shell[0]);
        strncpy(arg, "4", 2);
        f_write(arg, (void *)&shell[0]);
        f_close(NULL, (void *)&shell[0]);
        pthread_mutex_lock(&disk_mutex);
        printf("Appending process done %d\n", i);
        pthread_mutex_unlock(&disk_mutex);
    }
    return NULL;
}

void *thread2_func(void *) {
    char arg[256];
    usleep(500);
    pthread_mutex_lock(&disk_mutex);
    pthread_mutex_unlock(&disk_mutex);
    for (int i = 0; i < 16; i++) {
        pthread_mutex_lock(&disk_mutex);
        printf("Process 1 try to read %d\n", i);
        pthread_mutex_unlock(&disk_mutex);
        strncpy(arg, "a 0", 4);
        f_open(arg, (void *)&shell[1]);
        strncpy(arg, "-2", 3);
        f_read(arg, (void *)&shell[1]);
        f_close(NULL, (void *)&shell[1]);
        pthread_mutex_lock(&disk_mutex);
        printf("Process 1 done %d\n", i);
        pthread_mutex_unlock(&disk_mutex);
    }
    return NULL;
}

void *thread3_func(void *) {
    char arg[256];
    usleep(500);
    pthread_mutex_lock(&disk_mutex);
    pthread_mutex_unlock(&disk_mutex);
    for (int i = 0; i < 16; i++) {
        pthread_mutex_lock(&disk_mutex);
        printf("Process 2 try to read %d\n", i);
        pthread_mutex_unlock(&disk_mutex);
        strncpy(arg, "a 0", 4);
        f_open(arg, (void *)&shell[2]);
        strncpy(arg, "-2", 3);
        f_read(arg, (void *)&shell[2]);
        f_close(NULL, (void *)&shell[2]);
        pthread_mutex_lock(&disk_mutex);
        printf("Process 2 done %d\n", i);
        pthread_mutex_unlock(&disk_mutex);
    }
    return NULL;
}

int main(){
    freopen("/dev/urandom", "r", stdin);
    disk.mutex = &disk_mutex;
    setbuf(stdin, 0);
    setbuf(stdout, 0);
    if (init(&disk)) {
        printf("Init failed.\n");
        return -1;
    }
    
    for (int i = 0; i < 3; i++) {
        startup(&shell[i], &disk);
    }

    // 模拟多进程
    pthread_t t1, t2, t3;

    pthread_create(&t1, NULL, thread1_func, NULL);
    pthread_create(&t2, NULL, thread2_func, NULL);
    pthread_create(&t3, NULL, thread3_func, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_mutex_lock(&disk_mutex);
    pthread_mutex_unlock(&disk_mutex);

    printf("All process exit\n");

    return 0;
}