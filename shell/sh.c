#include "defs.h"
#include "types.h"
#include "readline.h"
#include "runcmd.h"

char prompt[PRMTLEN] = { 0 };
pid_t bg_pgid = 0;
int bg_process_count = 0;

// runs a shell command
static void
run_shell()
{
	char *cmd;

	while ((cmd = read_line(prompt)) != NULL)
		if (run_cmd(cmd) == EXIT_SHELL)
			return;
}

// initializes the shell
// with the "HOME" directory
static void
init_shell()
{
	stack_t ss;
	ss.ss_sp = malloc(SIGSTKSZ);
	if (ss.ss_sp == NULL) {
		perror("malloc");
		exit(1);
	}
	ss.ss_size = SIGSTKSZ;
	ss.ss_flags = 0;
	if (sigaltstack(&ss, NULL) == -1) {
		perror("sigaltstack");
		exit(1);
	}

	char buf[BUFLEN] = { 0 };
	char *home = getenv("HOME");

	if (chdir(home) < 0) {
		snprintf(buf, sizeof buf, "cannot cd to %s ", home);
		perror(buf);
	} else {
		char *buf = malloc(120);
		if (getcwd(buf, 120) == NULL) {
			perror("getcwd");
		}
		setenv("PWD", buf, 1);
		snprintf(prompt, sizeof prompt, "(%s)", home);
	}

	if (getenv("HISTFILE") == NULL) {
		// set the default history file as HOME/.fisop_history
		char history_file_path[FNAMESIZE];
		snprintf(history_file_path, sizeof history_file_path, "%s/.fisop_history", home);
		setenv("HISTFILE", history_file_path, 1);		
	}
}

static void
sigchld_handler(int)
{
	pid_t pid;
	int status;

	while ((pid = waitpid(-bg_pgid, &status, WNOHANG)) > 0) {
		printf_debug("==> terminado: PID=%d\n", pid);
		bg_process_count--;
	}

	// Esto se tuvo que agregar debido a que el pgid se volvia invalido al
	// terminar todos los procesos con el mismo pgid
	if (bg_process_count == 0) {
		bg_pgid = 0;
	}
}

int
main(void)
{
	struct sigaction sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = &sigchld_handler;
	sa.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sa, 0);

	init_shell();

	run_shell();

	return 0;
}
