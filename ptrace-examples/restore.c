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
char *str_val = NULL;

void proc_child()
{
    // Child process: Request tracing by the parent
    ptrace(PTRACE_TRACEME, 0, NULL, NULL);
    raise(SIGSTOP);

    val = 0x12121212;
    char *new_str = "new value";
    strncpy(str_val, new_str, strlen(new_str) + 1);
    printf("child: new val: %s\n", str_val); 
    raise(SIGSTOP);
    
    printf("child: res val: %s\n", str_val); 

    exit(0);
}

struct mem_region {
    unsigned long addr;
    unsigned int size;
};

int main() 
{
    str_val = malloc(100 * sizeof(char));
    char *mystr = "initial value";
    strncpy(str_val, mystr, strlen(mystr) + 1);
    char str_buf[100];
    int status, buf;
    pid_t child;
    struct mem_region cur;

    child = fork();
    if (child == 0)
        proc_child();

    // Parent process: Wait for the child to stop
    waitpid(child, &status, 0);
    cur.addr = str_val;
    cur.size = strlen(mystr) + 1;
    if (-1 == ptrace(PTRACE_SNAPSHOT, child, NULL, &cur)) {
        perror("ptrace PTRACE_SNAPSHOT");
        return 1;
    }
    printf("(snap)\n");
    buf = 0;
    if (-1 == ptrace(PTRACE_GETSNAPSHOT, child, str_val, str_buf)) {
        perror("ptrace PTRACE_GETSNAPSHOT");
        return 1;
    }
    printf("(getsnap) buf: %s\n", str_buf);
    ptrace(PTRACE_CONT, child, NULL, NULL);

    waitpid(child, &status, 0);
    if (-1 == ptrace(PTRACE_RESTORE, child, str_val, NULL)) {
        perror("ptrace PTRACE_GETSNAPSHOT");
        return 1;
    }
    ptrace(PTRACE_CONT, child, NULL, NULL);


    ptrace(PTRACE_DETACH, child, NULL, NULL);
    return 0;
}
