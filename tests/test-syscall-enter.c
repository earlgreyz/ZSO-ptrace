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
    char buf[10];
    int ret;

    raise(SIGSTOP);
    // tracer should dup2 read end of a pipe to fd 4, but just before read()
    ret = read(expected_fd, buf, sizeof(buf));
    if (ret == -1) {
        perror("TRACEE: read failed\n");
        return 1;
    } else if (ret != 4) {
        fprintf(stderr, "TRACEE: unexpected read size %d (expected 4)\n", ret);
        return 1;
    }
    if (strncmp(buf, "okay", 4)) {
        fprintf(stderr, "TRACEE: invalid read content\n");
        return 1;
    }
    return 0;
}

int main() {
    pid_t child_pid;
    int p[2];
    int status;
    struct ptrace_dup2_to_remote args;

    child_pid = fork_and_call_child(child, (void*)5);
    if (child_pid == -1)
        return 1;
    if (pipe(p) == -1) {
        perror("TRACER: pipe");
        return 1;
    }
    if (ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL) == -1) {
        perror("TRACER: PTRACE_SYSCALL");
        return 1;
    }
    if (waitpid(child_pid, &status, 0) == -1) {
        perror("TRACER: waitpid");
        return -1;
    }
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "TRACER: not stopped?\n");
        return -1;
    }

    args.local_fd = p[0];
    args.remote_fd = 4;
    args.flags = 0;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args) != 4) {
        perror("TRACER: dup to remote");
        return 1;
    }
    args.local_fd = p[0];
    args.remote_fd = 5;
    args.flags = 0;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args) != 5) {
        perror("TRACER: dup to remote");
        return 1;
    }
    if (write(p[1], "okay", 4) != 4) {
        perror("TRACER: write");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
