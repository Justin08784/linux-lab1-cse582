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

int main() 
{
    int status;
    pid_t child;

    child = fork();
    if (child == 0)
        proc_child();

    while (1) {
        // Parent process: Wait for the child to stop
        waitpid(child, &status, 0);

        if (WIFEXITED(status))
            break;

        int rv = ptrace(PTRACE_PEEKDATA, child, &val, NULL);
        if (rv == -1) {
            perror("ptrace PTRACE_GETREGSET");
            return 1;
        }
        printf("val: %x\n", rv);
        ptrace(PTRACE_CONT, child, NULL, NULL);
    }

    ptrace(PTRACE_DETACH, child, NULL, NULL);
    return 0;
}
