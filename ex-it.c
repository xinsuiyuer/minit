/*
    ex-it - exec and be init <https://github.com/chazomaticus/minit>
    Copyright 2014, 2015 Charles Lindsay <chaz@chazomatic.us>

    This software is provided 'as-is', without any express or implied warranty.
    In no event will the authors be held liable for any damages arising from
    the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software in
       a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#if _POSIX_C_SOURCE < 199309L
#error ex-it requires _POSIX_C_SOURCE >= 199309
#endif

enum {
    E_ARGS = 0x101, // > 0xff (max exit code from child), & 0xff != 0.
    E_LOW,
    E_EXEC,
    E_UNKNOWN,
    E_SIGNALED_START, // Must be last, so we can add the signal number to it.
};


static int handle_child(pid_t search_pid) {
    int search_status = -1;

    int status;
    for(pid_t pid; (pid = waitpid(-1, &status, WNOHANG)) > 0; ) {
        if(pid == search_pid) {
            if(WIFEXITED(status))
                search_status = WEXITSTATUS(status);
            else if(WIFSIGNALED(status))
                search_status = E_SIGNALED_START + WTERMSIG(status);
            else
                search_status = E_UNKNOWN;
        }
    }

    return search_status;
}

static int wait_for_child(pid_t child_pid) {
    sigset_t receive_set;
    sigemptyset(&receive_set);
    sigaddset(&receive_set, SIGCHLD);

    int status;
    for(status = -1; status < 0; status = handle_child(child_pid))
        sigwaitinfo(&receive_set, NULL);
    return status;
}

static pid_t run_args(char *argv[], sigset_t child_mask) {
    pid_t pid = fork();
    if(pid == -1) {
        fprintf(stderr, "%s: fork: %s\n", argv[0], strerror(errno));
        exit(E_LOW);
    }

    if(pid == 0) {
        sigprocmask(SIG_SETMASK, &child_mask, NULL);
        execvp(argv[1], &argv[1]);

        fprintf(stderr, "%s: exec %s: %s\n", argv[0], argv[1],
                strerror(errno));
        exit(E_EXEC);
    }

    return pid;
}

static sigset_t block_signals(void) {
    sigset_t block_set;
    sigfillset(&block_set);

    sigset_t old_mask;
    sigprocmask(SIG_SETMASK, &block_set, &old_mask);

    return old_mask;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "%s: too few arguments\n", argv[0]);
        return E_ARGS;
    }

    return wait_for_child(run_args(argv, block_signals()));
}
