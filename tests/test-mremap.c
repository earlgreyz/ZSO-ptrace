#define _GNU_SOURCE
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

char zero_tab[0x1000] = { 0 };

int child(void *arg) {
    char *mem = arg;
    char *new_tab = (char*)0x31337000;

    memset(mem, 42, 0x1000);
    raise(SIGSTOP);
    // here parent should mremap the area
    if (memcmp(new_tab+0x1000, zero_tab, 0x1000)) {
        fprintf(stderr, "Unexpected content of the second page\n");
        return 1;
    }
    memset(zero_tab, 42, 0x1000);
    if (memcmp(new_tab, zero_tab, 0x1000)) {
        fprintf(stderr, "Unexpected content of the first page\n");
        return 1;
    }
    return 0;
}

int main() {
    pid_t child_pid;
    char *mem;
    struct ptrace_remote_mremap args;

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
    if (child_pid == -1)
        return 1;
    args.old_addr = (uint64_t)mem;
    args.old_size = 0x1000;
    args.new_addr = 0x31337000;
    args.new_size = 0x2000;
    args.flags = MREMAP_FIXED | MREMAP_MAYMOVE;
    if (ptrace(PTRACE_REMOTE_MREMAP, child_pid, NULL, &args) == -1) {
        perror("TRACER: remote mremap");
        return 1;
    }
    return continue_and_wait_for_exit0(child_pid);
}
