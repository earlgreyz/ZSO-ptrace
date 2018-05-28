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

char zero_tab[0x200000] = { 0 };

int child(void *arg) {
    char *test_tab = arg;
    raise(SIGSTOP);
    // now tracer should mmap memory at test_tab
    // this is a bit naive, but at least check if mapping succeeded
    if (memcmp(test_tab, zero_tab, 0x200000)) {
        fprintf(stderr, "TTRACEE: unexpected content\n");
        return 1;
    }
    // try also writing
    memset(test_tab, 0xcc, 0x200000);
    return 0;
}

int main() {
    pid_t child_pid;
    struct ptrace_remote_mmap args = {
        .addr = 0x31400000,
        .length = 0x200000,
        .prot = PROT_READ | PROT_WRITE,
        .flags = MAP_ANONYMOUS | MAP_FIXED | MAP_PRIVATE |
            MAP_HUGETLB | (21 << MAP_HUGE_SHIFT),
        .fd = -1,
        .offset = 0,
    };
    
    child_pid = fork_and_call_child(child, (char*)0x31400000);

    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args) != 0x31400000) {
        perror("TRACER: remote mmap");
        return 1;
    }

    return continue_and_wait_for_exit0(child_pid);
}
