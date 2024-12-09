#include "builtin.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "history.h"

// returns true if the 'exit' call
// should be performed
//
// (It must not be called from here)
int
exit_shell(char *cmd)
{
	char *copia_cmd = strdup(cmd);
	split_line(copia_cmd, ' ');
	if (strcmp(copia_cmd, "exit") == 0) {
		return 1;
	}
	return 0;
}

// returns true if "chdir" was performed
//  this means that if 'cmd' contains:
// 	1. $ cd directory (change to 'directory')
// 	2. $ cd (change to $HOME)
//  it has to be executed and then return true
//
//  Remember to update the 'prompt' with the
//  	new directory.
//
// Examples:
//  1. cmd = ['c','d', ' ', '/', 'b', 'i', 'n', '\0']
//  2. cmd = ['c','d', '\0']
int
cd(char *cmd)
{
	// Your code here
	char *copia_cmd = strdup(cmd);
	char *argumento = split_line(copia_cmd, ' ');

	if (strcmp(copia_cmd, "cd") == 0) {
		if (strlen(argumento) == 0) {
			if (chdir(getenv("HOME")) < 0) {
				printf_debug("ERROR: %i\n", errno);
				return 1;
			}
			char *buf = malloc(120);
			if (getcwd(buf, 120) == NULL) {
				perror("getcwd failed");
			}
			setenv("PWD", buf, 1);
			snprintf(prompt, sizeof prompt, "(%s)", buf);
			return 1;
		} else {
			char *newdir = argumento;
			if (chdir(newdir) < 0) {
				printf_debug("ERROR: %i\n", errno);
				return 1;
			}
			char *buf = malloc(120);
			if (getcwd(buf, 120) == NULL) {
				perror("getcwd failed");
			}
			setenv("PWD", buf, 1);
			snprintf(prompt, sizeof prompt, "(%s)", buf);
			return 1;
		}
	}

	return 0;
}

// returns true if 'pwd' was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
pwd(char *cmd)
{
	// Your code here
	char *copia_cmd = strdup(cmd);
	split_line(copia_cmd, ' ');
	if (strcmp(copia_cmd, "pwd") == 0) {
		printf("%s\n", getenv("PWD"));
		fflush(stdout);
		free(copia_cmd);
		return 1;
	}
	return 0;
}

// returns true if `history` was invoked
// in the command line
//
// (It has to be executed here and then
// 	return true)
int
history(char *cmd)
{
	// Your code here
	char *copia_cmd = strdup(cmd);
	char *argumento = split_line(copia_cmd, ' ');
	(void) split_line(copia_cmd, ' ');

	if (strcmp(copia_cmd, "history") == 0) {
		char *history_file_path = getenv("HISTFILE");
		if (history_file_path == NULL) {
			perror("Couldn't get history file: HISTFILE not set");
			return 1;
		}
		int commands_count = 0;
		char **commands = get_history(history_file_path, &commands_count);
		if (commands_count == -1) {
			perror("Error reading history file\n");
			return 1;
		}

		if (strlen(argumento) > 0) {
			int n = atoi(argumento);
			if (n > commands_count) {
				n = commands_count;
			}
			for (int i = commands_count - n; i < commands_count; i++) {
				printf("%s", commands[i]);
			}
		} else {
			for (int i = 0; i < commands_count; i++) {
				printf("%d  %s", i, commands[i]);
			}
		}
		for (int i = 0; i < commands_count; i++) {
			free(commands[i]);
		}
		free(commands);
		return 1;
	}
	return 0;
}
