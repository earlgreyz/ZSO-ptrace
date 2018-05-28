#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/mman.h>

#include "ptrace_remote.h"
#include "utils.h"

int child(void *arg) {
    char *mem = arg;

    memset(mem, 0, 0x1000);
    raise(SIGSTOP);
    // here parent should unmap the area
    mem[0] = 1;
    fprintf(stderr, "TRACEE: shouldn't got that far\n");
    return 1;
}



int main() {
    pid_t child_pid;
    int status;
    char *mem;
    struct ptrace_remote_munmap args;

    mem = mmap(NULL,
            0x1000,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0);
    if (mem == MAP_FAILED) {
        perror("TRACEE: mmap");
        return 1;
    }

    child_pid = fork_and_call_child(child, mem);

    args.addr = (uint64_t)mem;
    args.length = 0x1000;
    if (ptrace(PTRACE_REMOTE_MUNMAP, child_pid, NULL, &args) != 0) {
        perror("TRACER: remote munmap");
        return 1;
    }
    if (ptrace(PTRACE_CONT, child_pid, 0, 0) == -1) {
        perror("TRACER: ptrace 1");
        return 1;
    }

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("TRACER: waitpid 2");
        return 1;
    }
    if (!WIFSTOPPED(status) || WSTOPSIG(status) != SIGSEGV) {
        fprintf(stderr, "TRACER: child didn't got SEGV\n");
        return 1;
    }
    return 0;
}
