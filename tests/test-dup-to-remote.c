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
    int pipe_fd = (long)arg;
    int pipe2_fd;
    char buf[10];
    int ret;

    raise(SIGSTOP);
    // now tracer should dup read end of a pipe and write it to pipe_fd
    if (read(pipe_fd, &pipe2_fd, sizeof(pipe2_fd)) != sizeof(pipe2_fd)) {
        perror("TRACEE: read pipe_fd");
        return 1;
    }
    if (fcntl(pipe2_fd, F_GETFD) != 0) {
        fprintf(stderr, "TRECEE: unexpected flags on fd\n");
        return 1;
    }
    ret = read(pipe2_fd, buf, sizeof(buf));
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
    int p[2], p1[2];
    int remote_fd;
    long tmp;
    struct ptrace_dup_to_remote args;

    if (pipe(p1) == -1) {
        perror("TRACER: pipe");
        return 1;
    }
    tmp = p1[0];
    child_pid = fork_and_call_child(child, (void*)tmp);
    if (child_pid == -1)
        return 1;
    if (pipe(p) == -1) {
        perror("TRACER: pipe");
        return 1;
    }
    args.local_fd = p[0];
    args.flags = 0;
    if ((remote_fd=ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, &args)) == -1) {
        perror("TRACER: dup to remote");
        return 1;
    }
    if (write(p1[1], &remote_fd, sizeof(remote_fd)) != sizeof(remote_fd)) {
        perror("TRACER: write 1");
        return 1;
    }
    if (write(p[1], "okay", 4) != 4) {
        perror("TRACER: write 2");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
