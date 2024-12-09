#include "runcmd.h"
#include "utils.h"
#include <stdio.h>

int status = 0;
struct cmd *parsed_pipe;
extern pid_t bg_pgid;
extern int bg_process_count;

// Si falla, se imprime un mensaje de error pero no obstaculiza la ejecución del comando
void save_cmd_to_history(char *cmd) {
	char *history_file_path = getenv("HISTFILE");
	if (history_file_path == NULL) {
		perror("Couldn't get history file: HISTFILE not set");
		return;
	}
	// Lo abro en modo append
	FILE *history_file = fopen(history_file_path, "a");
	if (history_file == NULL) {
		printf_debug("Couldn't open %s", history_file_path);
		return;
	}
	if (fprintf(history_file, "%s\n", cmd) < 0) {
		perror("Error writing to history file");
	}
	fclose(history_file);
}

// runs the command in 'cmd'
int
run_cmd(char *cmd)
{
	pid_t p;
	struct cmd *parsed;

	// if the "enter" key is pressed
	// just print the prompt again
	if (cmd[0] == END_STRING)
		return 0;

	save_cmd_to_history(cmd);

	// "history" built-in call
	if (history(cmd))
		return 0;

	// "cd" built-in call
	if (cd(cmd)) {
		status = 0;
		return 0;
	}

	// "exit" built-in call
	if (exit_shell(cmd)) {
		status = 0;
		return EXIT_SHELL;
	}

	// "pwd" built-in call
	if (pwd(cmd)) {
		status = 0;
		return 0;
	}

	// parses the command line
	parsed = parse_line(cmd);

	// forks and run the command
	if (parsed->type == BACK) {
		if (bg_pgid ==
		    0) {  // En primer ongoing bg process, se setea bg_pgid como el pid del proceso hijo
			if ((p = fork()) == 0) {
				setpgid(0, 0);  // pgid = pid
				exec_cmd(parsed);
				return 0;
			}
			bg_pgid = p;  //  bg_pgid = pid del proceso hijo
			printf("PID=%d\n", p);
			bg_process_count++;
			return 0;
		} else {  // Si el proceso no es el primer ongoing bg process
			if ((p = fork()) == 0) {
				setpgid(0, bg_pgid);  // pgid = bg_pgid
				exec_cmd(parsed);
				return 0;
			}
			printf("PID=%d\n", p);
			bg_process_count++;
			return 0;
		}
	} else {
		// keep a reference
		// to the parsed pipe cmd
		// so it can be freed later
		if ((p = fork()) == 0) {
			if (parsed->type == PIPE) {
				parsed_pipe = parsed;
			}
			exec_cmd(parsed);
		}
		// stores the pid of the process
		parsed->pid = p;

		// background process special treatment
		// Hint:
		// - check if the process is
		//		going to be run in the 'back'
		// - print info about it with
		// 	'print_back_info()'
		//
		// Your code here

		// waits for the process to finish
		waitpid(p, &status, 0);

		if (WIFEXITED(status)) {
			status = WEXITSTATUS(status);
		}
		// free_command(parsed); //esto lo comente yo

		return 0;
	}
}
