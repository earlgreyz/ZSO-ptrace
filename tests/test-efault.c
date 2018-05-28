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

int child(void *arg __attribute__((unused))) {
    raise(SIGSTOP);
    return 0;
}

int main() {
    pid_t child_pid;
    
    child_pid = fork_and_call_child(child, NULL);

    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MMAP should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_REMOTE_MUNMAP, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MUNMAP should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_REMOTE_MREMAP, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MREMAP should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_REMOTE_MPROTECT, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MPROTECT should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_DUP_TO_REMOTE should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_DUP2_TO_REMOTE should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_DUP_FROM_REMOTE should EFAULT\n");
        return 1;
    }
    if (ptrace(PTRACE_REMOTE_CLOSE, child_pid, NULL, 0x31337000) != -1
            || errno != EFAULT) {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_CLOSE should EFAULT\n");
        return 1;
    }

    return continue_and_wait_for_exit0(child_pid);
}
