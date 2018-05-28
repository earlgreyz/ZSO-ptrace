#define _GNU_SOURCE
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

int child(void *arg __attribute__((unused))) {

    raise(SIGSTOP);
    // now tracer duplicate fd then close it
    raise(SIGSTOP);
    return 0;
}

int main() {
    pid_t child_pid;
    int p[2];
    char buf[4];
    int fd, fd2;
    struct ptrace_remote_close args_close;
    struct ptrace_dup_to_remote args_dup;
    struct ptrace_dup2_to_remote args_dup2;
    struct ptrace_dup_from_remote args_dup_from;
    struct ptrace_remote_mmap args_mmap;

    child_pid = fork_and_call_child(child, (void*)4);
    if (child_pid == -1)
        return 1;
    if (pipe2(p, O_NONBLOCK) == -1) {
        perror("TRACER: pipe");
        return 1;
    }
    /* try various operations and in the end check if all references were
     * released, by expecting an EOF (instead of EAGAIN) */
    args_dup2.local_fd = p[1];
    args_dup2.remote_fd = 42;
    args_dup2.flags = 123;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args_dup2) != -1) {
        perror("TRACER: PTRACE_DUP2_TO_REMOTE should have failed");
        return 1;
    }
    args_dup2.local_fd = p[1];
    args_dup2.remote_fd = 42;
    args_dup2.flags = 0;
    if ((fd=ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args_dup2)) == -1) {
        perror("TRACER: PTRACE_DUP2_TO_REMOTE");
        return 1;
    }

    args_dup.local_fd = p[1];
    args_dup.flags = 123;
    if (ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, &args_dup) != -1) {
        perror("TRACER: PTRACE_DUP_TO_REMOTE should have failed");
        return 1;
    }
    args_dup.local_fd = p[1];
    args_dup.flags = 0;
    if ((fd=ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, &args_dup)) == -1) {
        perror("TRACER: PTRACE_DUP_TO_REMOTE");
        return 1;
    }


    args_dup_from.remote_fd = fd;
    args_dup_from.flags = 132;
    if (ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, &args_dup_from) != -1) {
        perror("TRACER: PTRACE_DUP_FROM_REMOTE should have failed");
        return 1;
    }
    args_dup_from.remote_fd = fd;
    args_dup_from.flags = 0;
    if ((fd2=ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, &args_dup_from)) == -1) {
        perror("TRACER: PTRACE_DUP_FROM_REMOTE");
        return 1;
    }
    close(fd2);

    args_mmap.addr = 0;
    args_mmap.length = 0x1000;
    args_mmap.prot = PROT_WRITE;
    args_mmap.flags = MAP_PRIVATE;
    args_mmap.fd = p[1];
    args_mmap.offset = 0;
    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args_mmap) != -1 || errno != EACCES) {
        perror("TRACER: PTRACE_REMOTE_MMAP should have failed with EACCES (fd is a pipe)");
        return 1;
    }

    args_close.remote_fd = 42;
    if (ptrace(PTRACE_REMOTE_CLOSE, child_pid, NULL, &args_close) == -1) {
        perror("TRACER: remote close 42");
        return 1;
    }
    args_close.remote_fd = fd;
    if (ptrace(PTRACE_REMOTE_CLOSE, child_pid, NULL, &args_close) == -1) {
        perror("TRACER: remote close");
        return 1;
    }
    close(p[1]);

    if (continue_and_wait_for_stop(child_pid))
        return -1;

    if (read(p[0], buf, 4) != 0) {
        perror("TRACER: read - expected EOF");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
