#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/ptrace.h>


int continue_and_wait_for_exit0(pid_t child_pid) {
    int status;

    if (ptrace(PTRACE_CONT, child_pid, 0, 0) == -1) {
        perror("TRACER: ptrace 1");
        return -1;
    }

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("TRACER: waitpid 2");
        return -1;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "TRACER: Child not exited?!\n");
        return -1;
    }
    if (WEXITSTATUS(status)) {
        fprintf(stderr, "TRACER: Child exited with error: %d\n", WEXITSTATUS(status));
        return -1;
    }
    return 0;
}

int continue_and_wait_for_stop(pid_t child_pid) {
    int status;

    if (ptrace(PTRACE_CONT, child_pid, 0, 0) == -1) {
        perror("TRACER: ptrace 1");
        return -1;
    }

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("TRACER: waitpid 2");
        return -1;
    }

    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "TRACER: not stopped?\n");
        return -1;
    }
    return 0;
}

/* fork, 
 *   child: call child()
 *   parent: wailt for child stop and return
 */
pid_t fork_and_call_child(int (*child)(void *), void *arg) {
    pid_t child_pid;
    int status;
    
    child_pid = fork();
    switch (child_pid) {
        case -1:
            perror("TRACER: fork");
            return -1;
        case 0:
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            exit(child(arg));
        default:
            break;
    }

    if (waitpid(child_pid, &status, 0) == -1) {
        perror("TRACER: waitpid");
        return -1;
    }
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "TRACER: not stopped?\n");
        return -1;
    }
    if (ptrace(PTRACE_SETOPTIONS, child_pid, PTRACE_O_EXITKILL, NULL) == -1) {
        perror("TRACER: PTRACE_SETOPTIONS");
        return -1;
    }
    return child_pid;
}
