#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#ifndef DEFAULT_STARTUP
    #define DEFAULT_STARTUP "/etc/rc.startup"
#endif


static volatile int terminate = 0;


static void handle_termination(int sig) {
    terminate = 1;
}

static sigset_t setup_signals(sigset_t *out_default_mask) {
    sigset_t wait_mask;
    sigfillset(&wait_mask);

    // In Linux and I believe most modern Unices, explicitly ignoring SIGCHLD
    // is enough to have children automatically reaped by the kernel.  If you
    // need this to work in an exotic environment, you'll want to instead
    // install your own handler that reaps children.
    sigaction(SIGCHLD, &(struct sigaction){ .sa_handler = SIG_IGN }, NULL);

    struct sigaction action = { .sa_handler = handle_termination };
    sigfillset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigdelset(&wait_mask, SIGTERM);
    sigdelset(&wait_mask, SIGINT);

    // TODO: also handle SIGUSR1 or such and run another script?

    sigset_t all_mask;
    sigfillset(&all_mask);
    sigprocmask(SIG_SETMASK, &all_mask, out_default_mask);

    return wait_mask;
}

static pid_t run(const char *filename, sigset_t mask) {
    pid_t pid = fork();
    // FIXME: handle errors.

    if(pid == 0) {
        sigprocmask(SIG_SETMASK, &mask, NULL);
        execlp(filename, filename, NULL);
        exit(errno == ENOENT ? 0 : 1);
    }

    return pid;
}

int main(int argc, char *argv[]) {
    sigset_t default_mask;
    sigset_t wait_mask = setup_signals(&default_mask);

    const char *startup = (argc > 1 ? argv[1] : NULL);
    run((startup && startup[0] ? startup : DEFAULT_STARTUP), default_mask);

    while(!terminate)
        sigsuspend(&wait_mask);

    // Only bring everything else down when we're actually init.
    if(getpid() == 1)
        kill(-1, SIGTERM);

    // TODO: run a shutdown script?

    return 0;
}
