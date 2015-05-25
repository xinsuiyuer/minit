/*
    minit - minimalist init implementation for containers
            <https://github.com/chazomaticus/minit>
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
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cmd.h"

#if _POSIX_C_SOURCE < 199309L
#error minit requires _POSIX_C_SOURCE >= 199309
#endif

#ifndef DEFAULT_STARTUP
#define DEFAULT_STARTUP "/etc/minit/startup"
#endif
#ifndef DEFAULT_SHUTDOWN
#define DEFAULT_SHUTDOWN "/etc/minit/shutdown"
#endif

#ifdef DEBUG
#define MLOG(FMT,...) \
  do { printf("%s:%d: "FMT"\n", __FILE__, __LINE__, ##__VA_ARGS__); fflush(stdout); } while(0)
#else
#define MLOG(FMT,...) \
  do { printf(FMT"\n", ##__VA_ARGS__); fflush(stdout); } while(0)
#endif

static const char *const default_startup = DEFAULT_STARTUP;
static const char *const default_shutdown = DEFAULT_SHUTDOWN;

static int   g_nstartup_cmd      = 0;
static cmd_t g_startup_cmds[256] = {
  { 0, 0, NULL, NULL, 0, NULL }
};
static int   g_nshutdown_cmd      = 0;
static cmd_t g_shutdown_cmds[256] = {
  { 0, 0, NULL, NULL, 0, NULL }
};
const size_t MAX_CMDS = sizeof(g_startup_cmds) / sizeof(g_startup_cmds[0]);

static void wait_for_child(const pid_t child_pid) {
    while(waitpid(child_pid, NULL, 0) > 0)
      continue;
}

static pid_t run(const char *filename, char* const args[],
                 sigset_t child_mask, int daemon) {
  char msg[512] = { 0 };
  pid_t pid = fork();
  if(pid == -1) {
    snprintf(msg, sizeof(msg) - 1, "[error] fork() %s failed:", filename);
    perror(msg);
  }

  if(pid == 0) {
    sigprocmask(SIG_SETMASK, &child_mask, NULL);
    int res = execv(filename, args);
    if(-1 == res) {
      snprintf(msg, sizeof(msg) - 1, "[error] execv() %s failed:", filename);
      perror(msg);
      exit(1);
    }

    exit(0);
  }

  if(daemon) {
    setpgid(pid, pid);
  }
  return pid;
}

static void start_cmds_in_order(const char* path, sigset_t child_mask) {
  if(!parse_cmds(path, MAX_CMDS, &g_nstartup_cmd, g_startup_cmds)) {
    return;
  }

  for(cmd_t *cmd = g_startup_cmds; cmd->exe; ++cmd) {
    pid_t pid = 0;
    MLOG("[startup] starting [%s] ...", cmd->exe);
    pid = run(cmd->exe, cmd->argv, child_mask, 1);
    if(pid <= 0) {
      MLOG("[startup] [%s] startup failed ...", cmd->exe);
      break;
    } else {
      cmd->pid = pid;
      MLOG("[startup] [%s(PID: %d)] is runing ...", cmd->exe, pid);
    }
  }

}

static void stop_cmds_in_reverse(const char* path, sigset_t child_mask) {
  (void)path;
  (void)child_mask;

  for(cmd_t *cmd = &g_startup_cmds[g_nstartup_cmd - 1]; cmd->exe; --cmd) {
    if(cmd->exited) continue;

    MLOG("[shutdown] stopping [%s(PID: %d)] ...", cmd->exe, cmd->pid);
    kill(cmd->pid, SIGINT);

    MLOG("[shutdown] waiting for [%s(PID: %d)] exit ...", cmd->exe, cmd->pid);
    wait_for_child(cmd->pid);
  }

  if(!parse_cmds(path, MAX_CMDS, &g_nshutdown_cmd, g_shutdown_cmds)) {
    return;
  }

  for(cmd_t *cmd = &g_shutdown_cmds[g_nshutdown_cmd - 1]; cmd->exe; --cmd) {
    if(!cmd->exe) break;

    MLOG("[cleanup] starting [%s] ...", cmd->exe);
    pid_t pid = run(cmd->exe, cmd->argv, child_mask, 0);
    if(-1 == pid) continue;
    MLOG("[cleanup] waiting for [%s(PID: %d)] exit ...", cmd->exe, pid);
    wait_for_child(pid);
  }
  
}

static void print_sigchld_details(const cmd_t *cmd, const siginfo_t *info) {
  pid_t pid = info->si_pid;
  int code  = info->si_code;
  char msg[512] = { 0 };
  const char* exe = "unrecognized child";
  if(cmd) {
    exe = cmd->exe;
  }

  char * next = msg + sprintf(msg, "[status] %s(PID: %d): ", exe, pid);
  switch(code) {
    case CLD_EXITED:
      strcat(next, "child has exited.");
      break;
    case CLD_KILLED:
      strcat(next, "child was killed.");
      break;
    case CLD_DUMPED:
      strcat(next, "child terminated abnormally."); // restart ?
      break;
    case CLD_TRAPPED:
      strcat(next, "child has trapped.");
      break;
    case CLD_STOPPED:
      strcat(next, "child has stopped.");
      break;
    case CLD_CONTINUED:
      strcat(next, "child has continued.");
      break;
    default:
      strcat(next, "unknown signal code.");
  } // end switch

  MLOG("%s", msg);
}

static int has_terminated(int code) {
  return (code == CLD_EXITED) || (code == CLD_KILLED) || (code == CLD_DUMPED);
}

static void handle_sigchld(const siginfo_t *info) {
      cmd_t *cmd = find_cmd_by_pid(g_startup_cmds, info->si_pid);
      print_sigchld_details(cmd, info);
      if(cmd && has_terminated(info->si_code))
        cmd->exited = 0; /* the child was terminated. */
}

static void wait_for_termination() {
  sigset_t receive_set;
  sigemptyset(&receive_set);
  /*sigaddset(&receive_set, SIGCHLD);*/
  sigaddset(&receive_set, SIGTERM);
  sigaddset(&receive_set, SIGINT);

  /* check every 100ms */
  /*const struct timespec ts = { 0, 100 * 1000000 };*/
  const struct timespec ts = { 1, 0 };

  int keep = 1;
  siginfo_t info;
  for(int signal; keep && (signal = sigtimedwait(&receive_set, &info, &ts)); ) {
    if((SIGTERM == signal) || (SIGINT == signal)) {
      MLOG("[status] minit received signal.");
      break;
    }

    while(1) {
      info.si_pid = 0;
      int res = waitid(P_ALL, 0, &info, WEXITED|WSTOPPED|WCONTINUED|WNOHANG);
      if(0 == res) {
        // call on success
        if(0 != info.si_pid) { handle_sigchld(&info); }
        else { break; /* nothing changed */ }
      } else {
        if(errno == ECHILD) {
          keep = 0; /* does not have any unwaited-for children */
        }
        break;
      }
    } /* end while */
  }

}

static sigset_t block_signals(void) {
  sigset_t block_set;
  sigfillset(&block_set);

  sigset_t old_mask;
  sigprocmask(SIG_SETMASK, &block_set, &old_mask);

  return old_mask;
}

int main(int argc, char *argv[]) {

  MLOG("[status] minit PID: %d", getpid());

  sigset_t default_mask = block_signals();

  const char *startup = (argc > 1 && *argv[1] ? argv[1] : default_startup);
  const char *shutdown = (argc > 2 && *argv[2] ? argv[2] : default_shutdown);

  start_cmds_in_order(startup, default_mask);
  wait_for_termination();
  stop_cmds_in_reverse(shutdown, default_mask);

  for(cmd_t *cmd = g_startup_cmds; cmd->exe; ++cmd) {
    free_cmd(cmd);
  }

  return 0;
}
