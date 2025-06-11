#include "main/shell.h"

#include "utils/utils.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void shell_exit() {
    printf("Exiting...\n");
    exit(0);
}

struct Command {
    char op[7], mode;
    uint32_t length;
    uint32_t argLen;
    void (* func)(char *, void *);
} commands[8] = {
    {"mkdir",   0,5,1,(void (* )(char *, void *))&mkdir},
    {"ls",      0,2,0,(void (* )(char *, void *))&ls},
    {"delete",  1,6,1,NULL},
    {"open",    0,5,1,NULL},
    {"read",    1,5,1,NULL},
    {"write",   1,5,1,NULL},
    {"close",   1,5,1,NULL},
    {"exit",    2,4,0,(void (* )(char *, void *))&shell_exit}
};

void print_prompt(Shell *shell) {
    printf("/");
    DirectoryBuffer *p = shell->buf[shell->index];
    while (p) {
        printf("%s/", p->dirname);
        p = p->child;
    }
    printf(" $ ");
}

char* read_command() {
    int c;
    char *buf = calloc(32, sizeof(char));
    if (!buf) {
        calloc_failed();
        exit(-1);
    }
    uint32_t cnt = 0;
    for (c = getchar(); c != '\n'; c = getchar()) {
        buf[cnt++] = (char)c;
        if ((cnt & 0x1f) == 0) {
            buf = realloc(buf, cnt + 32);
            if (!buf) {
                realloc_failed();
                exit(-1);
            }
        }
    }
    buf[cnt] = 0;
    return buf;
}

void process_command(char *buf, SysDirectory *dir) {
    char *arg = strstr(buf, " ");
    if (arg) {
        *arg = 0; arg++;
    }

    for (int i = 0; i < 8; i++) {
        if (!strncmp(commands[i].op, buf, commands[i].length)) {
            if ((!arg) && (commands[i].argLen > 0)) {
                printf("Missing arguments\n");
                return;
            }
            commands[i].func(arg, (void *)dir);
            return;
        }
    }
    printf("command not found: %s\n", buf);
}

void startup(Shell *shell, Disk *disk) {
    shell->partition = &(disk->sysmbr.partitions[0]);
    shell->dir = &(disk->sysmbr.partitions[0].root);
    shell->file = NULL;
    shell->buf[0] = NULL;
    shell->buf[1] = NULL;
    shell->buf[2] = NULL;
    shell->buf[3] = NULL;
    shell->index = shell->mode = 0;

    char *cmd;

    while (1) {
        print_prompt(shell);
        cmd = read_command();
        if (shell->mode) {}
        else {
            process_command(cmd, shell->dir);
        }

        free(cmd);
    }
}