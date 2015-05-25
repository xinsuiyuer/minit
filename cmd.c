
#include "cmd.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libiberty/libiberty.h>

static cmd_t* init_cmd(cmd_t *cmd) {
  cmd->pid    = 0;
  cmd->exited = 0;
  cmd->raw    = NULL;
  cmd->exe    = NULL;
  cmd->argc   = 0;
  cmd->argv   = NULL;
  return cmd;
}

void free_cmd(cmd_t *cmd) {
  if(cmd->raw) {
    free((void*)cmd->raw);
    freeargv(cmd->argv);
    init_cmd(cmd);
  }
}

static int parse_args(cmd_t *cmd, char *line) {
  char** argv = buildargv(line);
  int    argc = countargv(argv);

  if(0 == argc) {
    return 0; // this line is comment.
  }

  if('#' == *argv[0] || '\0' == *argv[0]) {
    freeargv(argv);
    return 0; // this line is comment.
  }

  cmd->argc = argc;
  cmd->argv = argv;
  cmd->exe       = cmd->argv[0];
  cmd->raw       = strdup(line);

  return 1;
}

int parse_cmds(const char* filename, const int max, int *ncmd, cmd_t cmds[]) {
  size_t len     = 0;
  ssize_t nread  = 0;
  char *line     = NULL;
  cmd_t *cmd = init_cmd(cmds);

  FILE *file = fopen(filename, "r");
  if(!file) {
    char msg[512] = { 0 };
    snprintf(msg, sizeof(msg) - 1, "[error] open '%s' failed:", filename);
    perror(msg);
    return 0;
  }

  while(((nread = getline(&line, &len, file)) != -1) && *ncmd <= max) {
    if(parse_args(cmd, line)) {
      ++(*ncmd);
      init_cmd(++cmd); // switch to parse next cmd
    }
  }
  return 1;
}

cmd_t* find_cmd_by_pid(cmd_t cmds[], const pid_t pid) {
  cmd_t *cmd = &cmds[0];
  while((pid != cmd->pid) && (0 != cmd->pid)) ++cmd;
  if(0 == cmd->pid) return NULL;

  return cmd;
}

