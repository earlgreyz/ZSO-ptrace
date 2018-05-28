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
    // now tracer should close write end of the pipe
    raise(SIGSTOP);
    return 0;
}

int main() {
    pid_t child_pid;
    int p[2];
    char buf[4];
    struct ptrace_remote_close args;

    if (pipe2(p, O_NONBLOCK) == -1) {
        perror("TRACER: pipe");
        return 1;
    }
    child_pid = fork_and_call_child(child, (void*)4);
    if (child_pid == -1)
        return 1;
    close(p[1]);
    args.remote_fd = p[1];
    if (ptrace(PTRACE_REMOTE_CLOSE, child_pid, NULL, &args) == -1) {
        perror("TRACER: remote close");
        return 1;
    }
    if (continue_and_wait_for_stop(child_pid))
        return -1;

    if (read(p[0], buf, 4) != 0) {
        perror("TRACER: read - expected EOF");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
