#ifndef RUNCMD_H
#define RUNCMD_H

#include "defs.h"
#include "parsing.h"
#include "exec.h"
#include "printstatus.h"
#include "freecmd.h"
#include "builtin.h"

void save_cmd_to_history(char *cmd);

int run_cmd(char *cmd);

#endif  // RUNCMD_H
