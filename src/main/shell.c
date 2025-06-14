#include "main/shell.h"

#include "utils/utils.h"
#include "fat16-partition/partition.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void shell_exit() {
    printf("Exiting...\n");
    exit(0);
}

void help() {
    puts("Commands:");
    puts("part <index>            : Change current partition");
    puts("                          index = 0, 1, 2, 3");
    puts("mkdir <dir name>        : Create a directory");
    puts("ls                      : Show all entities in current directory");
    puts("cd <dir name>           : Change work directory");
    puts("open <file name> <mode> : Open a file");
    puts("                          mode = 0: read-only mode ");
    puts("                                    (file must exists)");
    puts("                          mode = 1: read-write mode");
    puts("                          mode = 2: read-write-append mode");
    puts("read <n>                : Read n bytes from the file opened until end of file");
    puts("                          n = -1 means read from the first byte of the file");
    puts("write <n>               : Write n bytes to the file opened");
    puts("                          n = -1 means write from the first byte of the file");
    puts("delete <file/dir name>  : Delete a file or a directory if not empty");
    puts("help                    : Print this message");
    puts("exit                    : Exit shell");
}

#define COMMAND_CNT 11

struct Command {
    char op[8];
    uint32_t length;
    uint32_t argLen;
    void (* func)(char *, void *);
} commands[COMMAND_CNT] = {
    {"mkdir",   5,1,(void (* )(char *, void *))&mkdir},
    {"ls",      2,0,(void (* )(char *, void *))&ls},
    {"delete",  6,1,(void (* )(char *, void *))&delete},
    {"open",    5,1,(void (* )(char *, void *))&open},
    {"read",    5,1,(void (* )(char *, void *))&read},
    {"write",   5,1,(void (* )(char *, void *))&write},
    {"close",   5,0,(void (* )(char *, void *))&close},
    {"exit",    4,0,(void (* )(char *, void *))&shell_exit},
    {"cd",      2,1,(void (* )(char *, void *))&cd},
    {"part",    4,1,(void (* )(char *, void *))&part},
    {"help",    4,0,(void (* )(char *, void *))&help}
};

void print_prompt(Shell *shell) {
    printf("/");
    DirChainNode *p = shell->buf[shell->index].head;
    while (p) {
        if (p->mode == 0) { 
            printf("%s/", p->name);
        }
        else {
            printf("%s", p->name);
        }
        p = p->next;
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

void process_command(char *buf, void *shell) {
    char *arg = strstr(buf, " ");
    if (arg) {
        *arg = 0; arg++;
    }

    for (int i = 0; i < COMMAND_CNT; i++) {
        if (!strncmp(commands[i].op, buf, commands[i].length)) {
            if ((!arg) && (commands[i].argLen > 0)) {
                printf("Missing arguments\n");
                return;
            }
            commands[i].func(arg, shell);
            return;
        }
    }
    printf("command not found: %s\n", buf);
}

void startup(Shell *shell, Disk *disk) {
    memcpy(shell->partitions, &(disk->sysmbr.partitions), 4*sizeof(SysPartition));
    memset(shell->file, 0, 4*sizeof(SysPartition));
    for (int i = 0; i < 4; i++) {
        memcpy(&(shell->dir[i]), &(disk->sysmbr.partitions[i].root), sizeof(SysDirectory));
        shell->buf[i].head = shell->buf[i].tail = NULL;
        shell->mode[i] = 0;
    }
    shell->index = 0;

    char *cmd;
    while (1) {
        print_prompt(shell);
        cmd = read_command();
        process_command(cmd, (void *)shell);
        free(cmd);
    }
}