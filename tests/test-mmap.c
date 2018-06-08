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
    char *test_tab = arg;
    raise(SIGSTOP);
    // now tracer should mmap 0x31337000
    if (memcmp(test_tab, zero_tab, 0x1000)) {
        fprintf(stderr, "TTRACEE: unexpected content\n");
        return 1;
    }
    // try also writing
    memset(test_tab, 0xcc, 0x1000);
    return 0;
}

int main() {
    pid_t child_pid;
    struct ptrace_remote_mmap args = {
        .addr = 0x31337000,
        .length = 0x1000,
        .prot = PROT_READ | PROT_WRITE,
        .flags = MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE,
        .fd = -1,
        .offset = 0,
    };
    
    child_pid = fork_and_call_child(child, (char*)0x31337000);

    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args)) {
        perror("TRACER: remote mmap");
        return 1;
    }
    if (args.addr != 0x31337000) {
        fprintf(stderr, "Got unexpected address from PTRACE_REMOTE_MMAP (expected 0x31337000): %#lx\n", args.addr);
        return 1;
    }

    return continue_and_wait_for_exit0(child_pid);
}
