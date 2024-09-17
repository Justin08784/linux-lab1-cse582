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
    char *v3 = "v3 (3rd value :: *BAD*)";

    printf("child: writing to buf: \"%s\"\n", v1); 
    strncpy(buf, v1, strlen(v1) + 1);
    raise(SIGSTOP);

    printf("child: writing to buf: \"%s\"\n", v2); 
    strncpy(buf, v2, strlen(v2) + 1);
    raise(SIGSTOP);

    printf("child: writing to buf: \"%s\"\n", v3); 
    strncpy(buf, v3, strlen(v3) + 1);
    raise(SIGSTOP);

    printf("child: printing buf: \"%s\"\n", buf);
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

    waitpid(child, &status, 0);
    printf("parent: snap buf\n");
    if (-1 == ptrace(PTRACE_SNAPSHOT, child, NULL, &mem_reg)) {
        perror("ptrace PTRACE_SNAPSHOT");
        return 1;
    } 
    ptrace(PTRACE_CONT, child, NULL, NULL);

    waitpid(child, &status, 0);
    printf("parent: snap buf\n");
    if (-1 == ptrace(PTRACE_SNAPSHOT, child, NULL, &mem_reg)) {
        perror("ptrace PTRACE_SNAPSHOT");
        return 1;
    } 
    ptrace(PTRACE_CONT, child, NULL, NULL);

    waitpid(child, &status, 0);
    printf("parent: restore buf\n");
    if (-1 == ptrace(PTRACE_RESTORE, child, buf, NULL)) {
        perror("ptrace PTRACE_RESTORE");
        return 1;
    }
    ptrace(PTRACE_CONT, child, NULL, NULL);

    ptrace(PTRACE_DETACH, child, NULL, NULL);
    return 0;
}
