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




int main() 
{
    pid_t child;
    struct iovec io;
    struct user_pt_regs regs;  // ARM64 specific registers structure

    child = fork();
    if (child == 0) {
        // Child process: Request tracing by the parent
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        // execl("/bin/ls", "ls", NULL);  // Replace with any executable that triggers a syscall
        raise(SIGSTOP);
        char *mystr = "123123\n";
        write(STDOUT_FILENO, mystr, 8);
        return 0;
    }


    int status;
    // Parent process: Wait for the child to stop on SIGSTOP
    waitpid(child, &status, 0);

    // Use PTRACE_SYSCALL to trace system calls
    ptrace(PTRACE_SYSCALL, child, NULL, NULL);


    while (1) {
        // Wait for the child to enter a system call
        waitpid(child, &status, 0);

        // Check if the child has exited
        if (WIFEXITED(status)) {
            break;
        }

        // Prepare the iovec structure for PTRACE_GETREGSET
        io.iov_base = &regs;
        io.iov_len = sizeof(regs);

        // Use PTRACE_GETREGSET to retrieve the register values
        int rv = ptrace(PTRACE_GETREGSET, child, NT_PRSTATUS, &io);

        if (rv == -1) {
            perror("ptrace PTRACE_GETREGSET");
            return 1;
        }

        // On ARM64, the syscall number is stored in the x8 register (regs.regs[8])
        printf("System call number: %llu\n", regs.regs[8]);

        // Let the child continue to the next system call
        ptrace(PTRACE_SYSCALL, child, NULL, NULL);
    }

    // Detach the child process after tracing all syscalls
    ptrace(PTRACE_DETACH, child, NULL, NULL);

    return 0;
}
