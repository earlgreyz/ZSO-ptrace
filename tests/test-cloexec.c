#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/mman.h>

#include "ptrace_remote.h"
#include "utils.h"

int child(void *arg) {
    int expected_fd = (long)arg;

    raise(SIGSTOP);
    // now tracer should dup2 read end of a pipe to fd 4
    if (fcntl(expected_fd, F_GETFD) != FD_CLOEXEC) {
        fprintf(stderr, "TRECEE: unexpected flags on fd\n");
        return 1;
    }
    return 0;
}

int main() {
    pid_t child_pid;
    int p[2];
    int fd;
    struct ptrace_dup_to_remote args_dup;
    struct ptrace_dup2_to_remote args_dup2;
    struct ptrace_dup_from_remote args_dup_from;

    child_pid = fork_and_call_child(child, (void*)4);
    if (child_pid == -1)
        return 1;
    if (pipe(p) == -1) {
        perror("TRACER: pipe");
        return 1;
    }
    args_dup2.local_fd = p[0];
    args_dup2.remote_fd = 4;
    args_dup2.flags = O_CLOEXEC;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args_dup2) != 4) {
        perror("TRACER: dup2 to remote");
        return 1;
    }
    args_dup.local_fd = p[0];
    args_dup.flags = O_CLOEXEC;
    if ((fd=ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, &args_dup)) == -1) {
        perror("TRACER: dup to remote");
        return 1;
    }
    args_dup_from.remote_fd = fd;
    args_dup_from.flags = O_CLOEXEC;
    if ((fd=ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, &args_dup_from)) == -1) {
        perror("TRACER: dup from remote");
        return 1;
    }
    if (fcntl(fd, F_GETFD) != FD_CLOEXEC) {
        fprintf(stderr, "TRECER: unexpected flags on fd\n");
        return 1;
    }

    return continue_and_wait_for_exit0(child_pid);
}
