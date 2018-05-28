#ifndef _UTILS_H
#define _UTILS_H

int continue_and_wait_for_exit0(pid_t child_pid);
int continue_and_wait_for_stop(pid_t child_pid);
pid_t fork_and_call_child(int (*child)(void *), void *arg);

#endif /* _UTILS_H */
