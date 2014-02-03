#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#ifndef DEFAULT_RC
    #define DEFAULT_RC "/etc/rc.init"
#endif


static volatile int terminate = 0;


static void handle_child(int sig) {
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0)
        continue;

    errno = saved_errno;
}

static void handle_termination(int sig) {
    terminate = 1;
}

static sigset_t setup_signals(sigset_t *out_default_mask) {
    sigset_t wait_mask;
    sigfillset(&wait_mask);

    struct sigaction action = { .sa_flags = SA_NOCLDSTOP };
    sigfillset(&action.sa_mask);

    action.sa_handler = handle_child;
    sigaction(SIGCHLD, &action, NULL);
    sigdelset(&wait_mask, SIGCHLD);

    action.sa_handler = handle_termination;
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
    sigdelset(&wait_mask, SIGTERM);
    sigdelset(&wait_mask, SIGINT);

    // TODO: also handle SIGUSR1 or such and send it to the rc process?

    sigset_t all_mask;
    sigfillset(&all_mask);
    sigprocmask(SIG_SETMASK, &all_mask, out_default_mask);

    return wait_mask;
}

static pid_t run(const char *filename, sigset_t child_mask) {
    pid_t pid = fork();
    // FIXME: handle errors.

    if(pid == 0) {
        sigprocmask(SIG_SETMASK, &child_mask, NULL);
        execlp(filename, filename, NULL);
        exit(errno == ENOENT ? 0 : 1);
    }

    return pid;
}

int main(int argc, char *argv[]) {
    sigset_t default_mask;
    sigset_t wait_mask = setup_signals(&default_mask);

    const char *rc = (argc > 1 ? argv[1] : NULL);
    run((rc && rc[0] ? rc : DEFAULT_RC), default_mask);

    while(!terminate)
        sigsuspend(&wait_mask);

    // Only bring everything else down when we're actually init.
    if(getpid() == 1)
        kill(-1, SIGTERM);

    return 0;
}
