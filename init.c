#include <stddef.h>
#include <unistd.h>
#include <signal.h>


static volatile int terminate = 0;


static void handle_termination(int sig) {
    terminate = 1;
}

static sigset_t setup_signals(void) {
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
    sigprocmask(SIG_SETMASK, &all_mask, NULL);

    return wait_mask;
}

int main(int argc, char *argv[]) {
    sigset_t wait_mask = setup_signals();

    // TODO: run a startup script.

    while(!terminate)
        sigsuspend(&wait_mask);

    // Only bring everything else down when we're actually init.
    if(getpid() == 1)
        kill(-1, SIGTERM);

    // TODO: run a shutdown script?

    return 0;
}
