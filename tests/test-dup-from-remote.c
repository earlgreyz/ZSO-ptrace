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
    int p[2];
    int ret;

    if (pipe(p) == -1) {
        perror("TRACEE: pipe");
        return 1;
    }
    if (dup2(p[1], expected_fd) != expected_fd) {
        perror("TRACEE: dup2");
        return 1;
    }
    raise(SIGSTOP);
    // now tracer should get write end of the pipe from expected_fd
    close(p[1]);
    close(expected_fd);
    ret = read(p[0], buf, sizeof(buf));
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
    int fd;
    struct ptrace_dup_from_remote args;

    child_pid = fork_and_call_child(child, (void*)6);
    if (child_pid == -1)
        return 1;
    args.remote_fd = 6;
    args.flags = 0;
    if ((fd=ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, &args)) == -1) {
        perror("TRACER: dup from remote");
        return 1;
    }
    if (fcntl(fd, F_GETFD) != 0) {
        fprintf(stderr, "TRECER: unexpected flags on fd\n");
        return 1;
    }
    if (write(fd, "okay", 4) != 4) {
        perror("TRACER: write");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
