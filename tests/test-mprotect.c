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

    raise(SIGSTOP);
    // here parent should mprotect the area
    mem[0] = 1;
    return 0;
}

int main() {
    pid_t child_pid;
    char *mem;
    struct ptrace_remote_mprotect args;

    mem = mmap(NULL,
            0x1000,
            PROT_READ,
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
    args.prot = PROT_READ | PROT_WRITE;
    if (ptrace(PTRACE_REMOTE_MPROTECT, child_pid, NULL, &args) != 0) {
        perror("TRACER: remote munmap");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
