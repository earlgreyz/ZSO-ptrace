#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/mman.h>
#include <elf.h>

#include "ptrace_remote.h"
#include "utils.h"

#define NEW_APP "true-static.bin"

int child(void *arg __attribute__((unused))) {
    raise(SIGSTOP);
    // now tracer should mmap new binary and switch rip
    fprintf(stderr, "TTRACEE: shouldn't reach this line\n");
    return 1;
}

int main() {
    pid_t child_pid;
    int fd;
    struct stat stat_buf;
    void *remote_map;
    struct ptrace_remote_mmap mmap_args = {
        .addr = 0,
        .length = 0,
        .prot = PROT_READ | PROT_EXEC,
        .flags = MAP_PRIVATE,
        .fd = -1,
        .offset = 0,
    };

    child_pid = fork_and_call_child(child, 0);

    if ((fd=open(NEW_APP, O_RDONLY)) == -1) {
        perror("TRACER: open");
        return 1;
    }
    if (fstat(fd, &stat_buf) == -1) {
        perror("TRACER: fstat");
        return 1;
    }
    mmap_args.fd = fd;
    mmap_args.length = (stat_buf.st_size + 0xfff) & ~0xfff;
    if ((remote_map=(void*)ptrace(
                    PTRACE_REMOTE_MMAP, child_pid, NULL, &mmap_args)) == MAP_FAILED) {
        perror("TRACER: remote map");
        return 1;
    }
    if (ptrace(PTRACE_POKEUSER, child_pid, RIP * sizeof(unsigned long), remote_map) == -1) {
        perror("TRACER: ptrace pokeuser");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
