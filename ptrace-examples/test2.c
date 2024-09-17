#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <stddef.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

/* Kernel API */
#define PTRACE_SNAPSHOT		  10
#define PTRACE_RESTORE		  11
#define PTRACE_GETSNAPSHOT	  12

struct mem_region {
    unsigned long addr;
    unsigned int size;
};

/* Globals */
char *buf = NULL;
const size_t buf_len = 64;

void proc_child()
{
    // request tracing by parent
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);

    char *v1 = "v1 (1st value)";
    char *v2 = "v2 (2nd value)";

    printf("child: writing to buf: \"%s\"\n", v1); 
    strncpy(buf, v1, strlen(v1) + 1);
    raise(SIGSTOP);

    printf("child: writing to buf: \"%s\"\n", v2); 
    strncpy(buf, v2, strlen(v2) + 1);
    raise(SIGSTOP);

    exit(0);
}

int main() 
{
    buf = calloc(buf_len, sizeof(char));
    if (!buf) {
        perror("buf malloc");
        return 1;
    }

    int status;
    pid_t child;
    const struct mem_region mem_reg = {
        .addr = (unsigned long) buf,
        .size = buf_len,
    };

    child = fork();
    if (!child)
        proc_child();

    char snap1[buf_len];
    char snap2[buf_len];

    waitpid(child, &status, 0);
    printf("parent: snap buf\n");
    if (-1 == ptrace(PTRACE_SNAPSHOT, child, NULL, &mem_reg)) {
        perror("ptrace PTRACE_SNAPSHOT");
        return 1;
    } 
    printf("parent: getsnap buf\n");
    if (-1 == ptrace(PTRACE_GETSNAPSHOT, child, buf, snap1)) {
        perror("ptrace PTRACE_GETSNAPSHOT");
        return 1;
    }
    ptrace(PTRACE_CONT, child, NULL, NULL);


    waitpid(child, &status, 0);
    printf("parent: snap buf\n");
    if (-1 == ptrace(PTRACE_SNAPSHOT, child, NULL, &mem_reg)) {
        perror("ptrace PTRACE_SNAPSHOT");
        return 1;
    } 
    printf("parent: getsnap buf\n");
    if (-1 == ptrace(PTRACE_GETSNAPSHOT, child, buf, snap2)) {
        perror("ptrace PTRACE_GETSNAPSHOT");
        return 1;
    }
    ptrace(PTRACE_CONT, child, NULL, NULL);


    ptrace(PTRACE_DETACH, child, NULL, NULL);

    fputs("\nparent: perform snap1 -> snap2 diff = {\n", stdout);
    printf("snap1: %s\n", snap1);
    printf("snap2: %s\n", snap2);
    fputs("idx | snap1 | snap2\n", stdout);
    for (size_t i = 0; i < buf_len; ++i) {
        char c1 = snap1[i];
        char c2 = snap2[i];
        if (c1 == c2)
            continue;
        printf("%3lu   %5c   %5c\n", i, c1, c2);
    }
    fputs("}\n", stdout);

    // for (size_t i = 0; i < buf_len; ++i) {
    //     printf("%2x", snap1[i]);
    //     putc(i == buf_len - 1 ? '\n' : ' ', stdout);
    // }
    // for (size_t i = 0; i < buf_len; ++i) {
    //     printf("%2c", snap1[i]);
    //     putc(i == buf_len - 1 ? '\n' : ' ', stdout);
    // }
    return 0;
}
