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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


enum {
    E_ARGS = 0x101, // > 0xff (max exit code from child), & 0xff != 0.
    E_LOW,
    E_EXEC,
    E_UNKNOWN,
    E_SIGNALED_START, // Must be last, so we can add the signal number to it.
};


static int wait_for_child(pid_t child_pid) {
    while(1) {
        int status;
        if(wait(&status) == child_pid) {
            if(WIFEXITED(status))
                return WEXITSTATUS(status);
            if(WIFSIGNALED(status))
                return E_SIGNALED_START + WTERMSIG(status);
            return E_UNKNOWN;
        }
    }
}

static pid_t run_args(char *argv[]) {
    pid_t pid = fork();
    if(pid == -1) {
        fprintf(stderr, "%s: fork: %s\n", argv[0], strerror(errno));
        exit(E_LOW);
    }

    if(pid == 0) {
        execvp(argv[1], &argv[1]);

        fprintf(stderr, "%s: exec %s: %s\n", argv[0], argv[1],
                strerror(errno));
        exit(E_EXEC);
    }

    return pid;
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "%s: too few arguments\n", argv[0]);
        return E_ARGS;
    }

    return wait_for_child(run_args(argv));
}
