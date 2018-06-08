#include <unistd.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <linux/audit.h>

#include "ptrace_remote.h"
#include "utils.h"

int child(void *arg __attribute__((unused))) {
    raise(SIGSTOP);
    return 0;
}

int main() {
    pid_t child_pid;
    void *mem;
    int fd;
    struct ptrace_remote_mmap args_mmap;
    struct ptrace_remote_munmap args_munmap;
    struct ptrace_remote_mremap args_mremap;
    struct ptrace_remote_mprotect args_mprotect;
    struct ptrace_dup_to_remote args_dup;
    struct ptrace_dup2_to_remote args_dup2;
    struct ptrace_dup_from_remote args_dup_from;
    struct ptrace_remote_close args_close;

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
    fd = open("/dev/null", O_RDONLY);

    child_pid = fork_and_call_child(child, (void*)4);
    if (child_pid == -1)
        return 1;

    /* not aligned addr */
    args_mmap.addr = 0x31337;
    args_mmap.length = 0x1000;
    args_mmap.prot = PROT_READ | PROT_WRITE;
    args_mmap.flags = MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED;
    args_mmap.fd = -1;
    args_mmap.offset = 0;
    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args_mmap) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MMAP should EINVAL (addr)\n");
        return 1;
    }

    /* missing MAP_PRIVATE */
    args_mmap.addr = 0;
    args_mmap.length = 0x1000;
    args_mmap.prot = PROT_READ | PROT_WRITE;
    args_mmap.flags = MAP_ANONYMOUS;
    args_mmap.fd = -1;
    args_mmap.offset = 0;
    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args_mmap) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MMAP should EINVAL (missing MAP_PRIVATE or MAP_SHARED)\n");
        return 1;
    }

    /* invalid fd */
    args_mmap.addr = 0;
    args_mmap.length = 0x1000;
    args_mmap.prot = PROT_READ | PROT_WRITE;
    args_mmap.flags = MAP_PRIVATE;
    args_mmap.fd = -1;
    args_mmap.offset = 0;
    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args_mmap) != -1
            || errno != EBADF)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MMAP should EBADF (invalid fd)\n");
        return 1;
    }

    /* incompatible prot */
    args_mmap.addr = 0;
    args_mmap.length = 0x1000;
    args_mmap.prot = PROT_READ | PROT_WRITE;
    args_mmap.flags = MAP_SHARED;
    args_mmap.fd = fd;
    args_mmap.offset = 0;
    if (ptrace(PTRACE_REMOTE_MMAP, child_pid, NULL, &args_mmap) != -1
            || errno != EACCES)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MMAP should EACCES (incompatible prot)\n");
        return 1;
    }

    /* invalid addr */
    args_mprotect.addr = 0x424242001;
    args_mprotect.length = 0x1000;
    args_mprotect.prot = PROT_READ | PROT_WRITE;
    if (ptrace(PTRACE_REMOTE_MPROTECT, child_pid, NULL, &args_mprotect) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MPROTECT should EINVAL (invalid addr)\n");
        return 1;
    }

    /* invalid new_size */
    args_mremap.old_addr = (long)mem;
    args_mremap.old_size = 0x1000;
    args_mremap.new_addr = 0;
    args_mremap.new_size = 0x1234;
    if (ptrace(PTRACE_REMOTE_MREMAP, child_pid, NULL, &args_mremap) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MREMAP should EINVAL (invalid new_size)\n");
        return 1;
    }

    /* invalid addr */
    args_munmap.addr = (long)mem + 1;
    args_munmap.length = 0x1000;
    if (ptrace(PTRACE_REMOTE_MUNMAP, child_pid, NULL, &args_munmap) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_MUNMAP should EINVAL (invalid addr)\n");
        return 1;
    }

    /* invalid local_fd */
    args_dup.local_fd = 14;
    args_dup.flags = 0;
    if (ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, &args_dup) != -1
            || errno != EBADF)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_TO_REMOTE should EBADF\n");
        return 1;
    }

    /* invalid flags */
    args_dup.local_fd = fd;
    args_dup.flags = 123;
    if (ptrace(PTRACE_DUP_TO_REMOTE, child_pid, NULL, &args_dup) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_TO_REMOTE should EINVAL (flags)\n");
        return 1;
    }

    /* invalid local_fd */
    args_dup2.local_fd = 14;
    args_dup2.remote_fd = 14;
    args_dup2.flags = 0;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args_dup2) != -1
            || errno != EBADF)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_TO_REMOTE should EBADF (local_fd)\n");
        return 1;
    }

    /* invalid remote_fd */
    args_dup2.local_fd = fd;
    args_dup2.remote_fd = -1;
    args_dup2.flags = 0;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args_dup2) != -1
            || errno != EBADF)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_TO_REMOTE should EBADF (remote_fd)\n");
        return 1;
    }

    /* invalid flags */
    args_dup2.local_fd = fd;
    args_dup2.remote_fd = 14;
    args_dup2.flags = 123;
    if (ptrace(PTRACE_DUP2_TO_REMOTE, child_pid, NULL, &args_dup2) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_TO_REMOTE should EINVAL (flags)\n");
        return 1;
    }

    /* invalid remote_fd */
    args_dup_from.remote_fd = -1;
    args_dup_from.flags = 0;
    if (ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, &args_dup_from) != -1
            || errno != EBADF)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_FROM_REMOTE should EBADF (remote_fd)\n");
        return 1;
    }

    /* invalid flags */
    args_dup_from.remote_fd = fd;
    args_dup_from.flags = 0x123;
    if (ptrace(PTRACE_DUP_FROM_REMOTE, child_pid, NULL, &args_dup_from) != -1
            || errno != EINVAL)  {
        fprintf(stderr, "TRACER: PTRACE_DUP_FROM_REMOTE should EINVAL (flags)\n");
        return 1;
    }

    /* invalid remote_fd */
    args_close.remote_fd = -1;
    if (ptrace(PTRACE_REMOTE_CLOSE, child_pid, NULL, &args_close) != -1
            || errno != EBADF)  {
        fprintf(stderr, "TRACER: PTRACE_REMOTE_CLOSE should EBADF (remote_fd)\n");
        return 1;
    }

    return 0;
}
