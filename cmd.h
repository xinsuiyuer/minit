#ifndef MINIT_CMD_H_
#define MINIT_CMD_H_

#include <unistd.h>

typedef struct cmd_t {
  pid_t pid;
  int exited;
  const char *raw;
  const char *exe;
  int argc;
  char** argv;
} cmd_t;

int parse_cmds(const char* filename, const int max, int *ncmd, cmd_t cmds[]);
cmd_t* find_cmd_by_pid(cmd_t cmds[], const pid_t pid);
void free_cmd(cmd_t *cmd);


#endif /* end of include guard: MINIT_COMMAND_H_ */
