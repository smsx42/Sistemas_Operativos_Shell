#include <unistd.h>

#include "printstatus.h"

// prints information of process' status
void
print_status_info(struct cmd *cmd)
{
	if (strlen(cmd->scmd) == 0 || cmd->type == PIPE)
		return;

	if (WIFEXITED(status)) {
		status = WEXITSTATUS(status);
	} else if (WIFSIGNALED(status)) {
		status = -WTERMSIG(status);
	} else if (WTERMSIG(status)) {
		status = -WSTOPSIG(status);
	} else {
		return;
	}

#ifndef SHELL_NO_INTERACTIVE
	if (isatty(1)) {
		const char *action;

		if (WIFEXITED(status)) {
			action = "exited";
		} else if (WIFSIGNALED(status)) {
			action = "killed";
		} else if (WTERMSIG(status)) {
			action = "stopped";
		}

		fprintf(stdout,
		        "%s	Program: [%s] %s, status: %d %s\n",
		        COLOR_BLUE,
		        cmd->scmd,
		        action,
		        status,
		        COLOR_RESET);
	}
#endif
}

// prints info when a background process is spawned
void
print_back_info(struct cmd *back __attribute__((unused)))
{
#ifndef SHELL_NO_INTERACTIVE
	if (isatty(1)) {
		fprintf(stdout, "%s  [PID=%d] %s\n", COLOR_BLUE, back->pid, COLOR_RESET);
	}
#endif
}
