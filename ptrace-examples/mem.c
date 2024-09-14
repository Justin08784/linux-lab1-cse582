#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>
#include <linux/ptrace.h>
#include <linux/uio.h>
#include <errno.h>
#include <linux/elf.h>
#include <signal.h>

#define PTRACE_SNAPSHOT		  10
#define PTRACE_RESTORE		  11
#define PTRACE_GETSNAPSHOT	  12

int val = 0x01010101;

void proc_child()
{
    // Child process: Request tracing by the parent
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    raise(SIGSTOP);

    val = 0x12121212;
    raise(SIGSTOP);

    val = 0xDEADBEEF;
    raise(SIGSTOP);

    exit(0);
}

struct mem_region {
    unsigned long addr;
    unsigned int size;
};

int main() 
{
    int status;
    pid_t child;

    child = fork();
    if (child == 0)
        proc_child();

    while (1) {
        // Parent process: Wait for the child to stop
        int rv;
        struct mem_region cur;
        waitpid(child, &status, 0);
        if (WIFEXITED(status))
            break;

        cur.addr = &val;
        cur.size = sizeof(int);
        rv = ptrace(PTRACE_SNAPSHOT, child, NULL, &cur);
        if (rv == -1) {
            perror("ptrace PTRACE_SNAPSHOT");
            return 1;
        }
        printf("(snap)rv: %d\n", rv);

        int jawohl = 0;
	printf("jawohl: %p\n", &jawohl);
        rv = ptrace(PTRACE_GETSNAPSHOT, child, &val, &jawohl);
        if (rv == -1) {
            perror("ptrace PTRACE_GETSNAPSHOT");
            return 1;
        }
        printf("(getsnap)rv: %d, humpty: %x\n", rv, jawohl);

        ptrace(PTRACE_CONT, child, NULL, NULL);
    }

    ptrace(PTRACE_DETACH, child, NULL, NULL);
    return 0;
}
