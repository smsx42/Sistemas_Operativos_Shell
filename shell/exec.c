#include "exec.h"
#include "defs.h"
#include "types.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// sets "key" with the key part of "arg"
// and null-terminates it
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  key = "KEY"
//
static void
get_environ_key(char *arg, char *key)
{
	int i;
	for (i = 0; arg[i] != '='; i++)
		key[i] = arg[i];

	key[i] = END_STRING;
}

// sets "value" with the value part of "arg"
// and null-terminates it
// "idx" should be the index in "arg" where "=" char
// resides
//
// Example:
//  - KEY=value
//  arg = ['K', 'E', 'Y', '=', 'v', 'a', 'l', 'u', 'e', '\0']
//  value = "value"
//
static void
get_environ_value(char *arg, char *value, int idx)
{
	size_t i, j;
	for (i = (idx + 1), j = 0; i < strlen(arg); i++, j++)
		value[j] = arg[i];

	value[j] = END_STRING;
}

// sets the environment variables received
// in the command line
//
// Hints:
// - use 'block_contains()' to
// 	get the index where the '=' is
// - 'get_environ_*()' can be useful here
static void
set_environ_vars(char **eargv, int eargc)
{
	int index_equal = 0;

	for (int i = 0; i < eargc; i++) {
		char *key;
		char *value;

		index_equal = block_contains(eargv[i], '=');  // Busca el =.

		if (index_equal == 0) {  // Si no se encontro el = deja de iterar.
			break;
		}

		key = malloc(index_equal + 1);
		if (!key) {
			printf_debug("Error en reservar memoria para la key.");
			break;
		}

		value = malloc(sizeof(eargv[i]) - index_equal);
		if (!value) {
			printf_debug(
			        "Error en reservar memoria para el value.");
			free(key);
			break;
		}


		// Setea la key y el value

		get_environ_key(eargv[i], key);
		get_environ_value(eargv[i], value, index_equal);

		setenv(key, value, 0);
		free(value);
		free(key);
	}
}

// opens the file in which the stdin/stdout/stderr
// flow will be redirected, and returns
// the file descriptor
//
// Find out what permissions it needs.
// Does it have to be closed after the execve(2) call?
//
// Hints:
// - if O_CREAT is used, add S_IWUSR and S_IRUSR
// 	to make it a readable normal file
static int
open_redir_fd(char *file, int flags)
{
	int fd;
	if (flags & O_CREAT) {
		fd = open(file,
		          O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC,
		          S_IRUSR | S_IWUSR);
	} else {
		fd = open(file, O_CLOEXEC, S_IRUSR | S_IWUSR);
	}

	if (fd == -1) {
		printf_debug("Error number %d\n", errno);
		return -1;
	}

	return fd;
}

static void
parse_command(char *new_command, char **argumentos)
{
	char *arg = malloc(strlen(new_command) * sizeof(char));
	int i = 0;
	while ((arg = split_line(new_command, ' '))) {
		if (new_command[0] == '\0') {
			break;
		}
		argumentos[i] = strdup(new_command);
		i++;
		new_command = strdup(arg);

		if (arg[0] == '\0') {
			break;
		}
	}
	argumentos[i] = '\0';
}

// executes a command - does not return
//
// Hint:
// - check how the 'cmd' structs are defined
// 	in types.h
// - casting could be a good option
void
exec_cmd(struct cmd *cmd)
{
	// printf_debug("entrando a exec_cmd\n");
	fflush(stdout);
	// To be used in the different cases
	struct execcmd *e;
	struct backcmd *b;
	struct execcmd *r;
	struct pipev2cmd *p;

	char *slice1;
	char *slice2;

	switch (cmd->type) {
	case EXEC: {
		slice1 = cmd->scmd;
		e = (struct execcmd *) cmd;
		set_environ_vars(e->eargv, e->eargc);
		int i = 0;
		while ((slice2 = split_line(slice1, ' '))) {
			// printf_debug("slice1: %s\n", slice1);
			//  printf_debug("slice2: %s\n", slice2);
			if (slice1[0] == '\0') {
				break;
			}
			e->eargv[i] = strdup(slice1);
			i++;
			slice1 = strdup(slice2);

			// ya no hay nada que partir
			if (slice2[0] == '\0') {
				break;
			}
		}

		if (execvp(e->argv[0], e->argv) < 0) {
			printf_debug("ERROR %i AL hacer exec\n", errno);
		}

		_exit(-1);
		break;
	}

	case BACK: {
		// runs a command in background
		b = (struct backcmd *) cmd;
		exec_cmd(b->c);
		break;
	}

	case REDIR: {
		// changes the input/output/stderr flow
		int new_fd_i;
		int new_fd_o;
		int new_fd_e;
		int status = 0;
		r = (struct execcmd *) cmd;


		char *output_file = r->out_file;
		char *input_file = r->in_file;
		char *error_file = r->err_file;


		if (strlen(output_file) > 0) {
			new_fd_o = open_redir_fd(output_file, O_CREAT);
			if (new_fd_o == -1) {
				_exit(-1);
			}
			status = dup2(new_fd_o, STDOUT_FILENO);
			if (status < 0) {
				_exit(-1);
			}
			close(new_fd_o);
		}


		if ((strlen(error_file) > 0) && (error_file[0] != '&')) {
			new_fd_e = open_redir_fd(error_file, O_CREAT);
			if (new_fd_e == -1) {
				_exit(-1);
			}
			status = dup2(new_fd_e, STDERR_FILENO);
			if (status < 0) {
				_exit(-1);
			}
			close(new_fd_e);
		}

		if (error_file[0] == '&') {
			dup2(STDOUT_FILENO, STDERR_FILENO);
		}

		if (strlen(input_file) > 0) {
			new_fd_i = open_redir_fd(input_file, O_RDONLY);
			if (new_fd_i == -1) {
				_exit(-1);
			}
			status = dup2(new_fd_i, STDIN_FILENO);
			if (status < 0) {
				_exit(-1);
			};
			close(new_fd_i);
		}

		if (execvp(r->argv[0], r->argv) < 0) {
			perror("execvp failed");
			_exit(-1);
		}

		exit(1);
		break;
	}

	case PIPE: {
		p = (struct pipev2cmd *) cmd;
		int num_pipes = p->cantidad_cmd - 1;
		int pipe_fds[num_pipes][2];
		int i;
		char *arguments[MAX_ARGUMENT];
		int status = 0;

		for (i = 0; i < num_pipes; i++) {
			if (pipe(pipe_fds[i]) == -1) {
				perror("pipe");
				exit(EXIT_FAILURE);
			}
		}

		for (i = 0; i < p->cantidad_cmd; i++) {
			pid_t pid = fork();
			if (pid == -1) {
				perror("fork");
				exit(-1);
			}

			if (pid == 0) {  // Proceso hijo
				if (i > 0) {  // Si no es el primer comando, toma entrada de la tubería anterior
					status = dup2(pipe_fds[i - 1][0],
					              STDIN_FILENO);
					if (status < 0) {
						printf_debug("ERROR %i AL "
						             "hacer dup2\n",
						             errno);
						exit(-1);
					};
				}
				if (i <
				    num_pipes) {  // Si no es el último comando, envía salida a la siguiente tubería
					status = dup2(pipe_fds[i][1],
					              STDOUT_FILENO);
					if (status < 0) {
						printf_debug("ERROR %i AL "
						             "hacer dup2\n",
						             errno);
						exit(-1);
					};
				}

				for (int j = 0; j < num_pipes; j++) {
					close(pipe_fds[j][0]);
					close(pipe_fds[j][1]);
				}

				parse_command(p->cmds[i]->scmd, arguments);
				if (execvp(arguments[0], arguments) < 0) {
					printf_debug("ERROR %i AL hacer exec\n",
					             errno);
					exit(-1);
				}
				exit(-1);
			}
		}

		for (i = 0; i < num_pipes; i++) {
			close(pipe_fds[i][0]);
			close(pipe_fds[i][1]);
		}

		for (i = 0; i < p->cantidad_cmd; i++) {
			wait(NULL);
		}
		exit(-1);
		break;
	}
	}
}
