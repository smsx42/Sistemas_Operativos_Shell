#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include "defs.h"
#include "readline.h"
#include "history.h"
#include <sys/ioctl.h>

struct termios saved_attributes;

void
reset_input_mode (void)
{
  tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void
set_input_mode (void)
{
  struct termios tattr;
//   char *name; 

  /* Make sure stdin is a terminal. */
  if (!isatty (STDIN_FILENO))
    {
      fprintf (stderr, "Not a terminal.\n");
      exit (EXIT_FAILURE);
    }

  /* Save the terminal attributes so we can restore them later. */
  tcgetattr (STDIN_FILENO, &saved_attributes);
  atexit (reset_input_mode);

  /* Set the funny terminal modes. */
  tcgetattr (STDIN_FILENO, &tattr);
  tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
  tattr.c_cc[VMIN] = 1;
  tattr.c_cc[VTIME] = 0;
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

static char buffer[BUFLEN];

// reads a line from the standard input
// and prints the prompt
char *
read_line(const char *prompt __attribute__((unused)))
{
#ifndef SHELL_NO_INTERACTIVE
	if (isatty(1)) {
		fprintf(stdout, "%s %s %s\n", COLOR_RED, prompt, COLOR_RESET);
		fprintf(stdout, "%s", "$ ");
	}
#endif
	size_t i = 0;
	int c = 0;
	if (!isatty(STDIN_FILENO)) { // canonical

		memset(buffer, 0, BUFLEN);

		c = getchar();

		while (c != END_LINE && c != EOF) {
			buffer[i++] = c;
			c = getchar();
		}

		// if the user press ctrl+D
		// just exit normally
		if (c == EOF)
			return NULL;

		buffer[i] = END_STRING;
		return buffer;
	} else { // non-canonical
		memset(buffer, 0, BUFLEN);
		char *history_file_path = getenv("HISTFILE");
		if (history_file_path == NULL) {
			perror("Couldn't get history file: HISTFILE not set");
			return NULL;
		}
		int commands_count = 0;
		char **commands = get_history(history_file_path, &commands_count);
		if (commands_count == -1) {
			return NULL;
		}
		size_t current_command_index = (int)commands_count;

		set_input_mode();

		c = getchar();
		while (c != END_LINE && c != EOF && c != 4) { // while c is not enter, EOF or ctrl+D
			if (c == 27) { // escape sequence
				escape_sequence(buffer, &i, &c, commands, commands_count, &current_command_index);
			} else
			
			if (c == 127) { // backspace
				backspace(buffer, &i);
			} else
			
			{ // character input
				insert_without_overwriting(buffer, &i, &c);
			}

			if (c != END_LINE && c != EOF && c != 4) { // get next character only if it's not enter, EOF or ctrl+D. This could happen if the user presses ctrl+D in a escape sequence
				c = getchar();
			}

		}
		clean_before_exit(commands, commands_count);
		if (c == 4) {			
			return NULL;
		}
		buffer[strlen(buffer)] = END_STRING;
		return buffer;
	}
}

void insert_without_overwriting(char *buffer, size_t *i, int *c) {
	empty_stdout(buffer, i);
	// reassign buffer
	for (size_t j = strlen(buffer); j > *i; j--) {
		buffer[j] = buffer[j - 1];
	}
	buffer[*i] = *c;
	// print buffer
	if (write(STDOUT_FILENO, buffer, strlen(buffer)) == -1) {
		perror("write failed");
	}
	(*i)++;
	// move cursor to new i
	for (size_t j = *i; j < strlen(buffer); j++) {
		if (write(STDOUT_FILENO, "\b", 1) == -1) {
			perror("write failed");
		}
	}
}

void backspace(char *buffer, size_t *i) {
	if (*i > 0) {
		empty_stdout(buffer, i);
		// reassign buffer
		for (size_t j = *i - 1; j < strlen(buffer); j++) {
			buffer[j] = buffer[j + 1];
		}
		// print buffer
		if (write(STDOUT_FILENO, buffer, strlen(buffer)) == -1) {
			perror("write failed");
		}
		// set i to the end of the buffer
		(*i)--;
	}
}

void escape_sequence(char *buffer, size_t *i, int *c, char **commands, int commands_count, size_t *current_command_index) {
	*c = getchar();
	if (*c == 91) { // [
		*c = getchar();
		if (*c == 65) { // UP
			previous_command(buffer, i, commands, commands_count, current_command_index);
		} else
		if (*c == 66) { // DOWN
			next_command(buffer, i, commands, commands_count, current_command_index);
		} else
		if (*c == 67) { // RIGHT
			move_cursor_right(buffer, i);
		} else
		if (*c == 68) { // LEFT
			move_cursor_left(buffer, i);
		} else
		if (*c == 72) { // HOME
			home(buffer, i);
		} else
		if (*c == 70) { // END
			// printf_debug("Entered if END i: %zu\n", *i);
			end(buffer, i);
		} else
		if (*c == 49) {
			*c = getchar();
			if (*c == 59) {
				*c = getchar();
				if (*c == 53) {
					*c = getchar();
					if (*c == 67) { // ctrl+right
						move_cursor_right_word(buffer, i);
					} else
					if (*c == 68) { // ctrl+left
						move_cursor_left_word(buffer, i);
					}
				}
			}
		}	
	}
}

void home(char *buffer __attribute__((unused)), size_t *i) {
	for (size_t j = 0; j < *i; j++) {
		if (write(STDOUT_FILENO, "\b", 1) == -1) {
			perror("write failed");
		}
	}
	*i = 0;
}

void end(char *buffer, size_t *i) {
	for (size_t j = *i; j < strlen(buffer); j++) {
		if (write(STDOUT_FILENO, buffer + j, 1) == -1) {
			perror("write failed");
		}
		(*i)++;
	}
}

void previous_command(char *buffer, size_t *i, char **commands, int commands_count __attribute__((unused)), size_t *current_command_index) {
	if (*current_command_index > 0) {
		(*current_command_index)--;
		empty_stdout(buffer, i);
		empty_buffer(buffer);
		// get previous command
		strcpy(buffer, commands[*current_command_index]);
		// strip off the newline
		buffer[strlen(buffer) - 1] = END_STRING;
		// print previous command
		if (write(STDOUT_FILENO, buffer, strlen(buffer)) == -1) {
			perror("write failed");
		}
		// set i to the end of the buffer
		*i = strlen(buffer);
	}
}

void next_command(char *buffer, size_t *i, char **commands, int commands_count, size_t *current_command_index) {
	if (*current_command_index < (size_t)commands_count) {
		(*current_command_index)++;
		empty_stdout(buffer, i);
		empty_buffer(buffer);
		if (*current_command_index == (size_t)commands_count) {
			// set i to 0
			*i = 0;
		} else {
			// get next command
			strcpy(buffer, commands[*current_command_index]);
			// strip off the newline
			buffer[strlen(buffer) - 1] = END_STRING;
			// print next command
			if (write(STDOUT_FILENO, buffer, strlen(buffer)) == -1) {
				perror("write failed");
			}
			// set i to the end of the buffer
			*i = strlen(buffer);
		}
	}
}

void move_cursor_right(char *buffer, size_t *i) {
	if (*i < strlen(buffer)) {
		if (write(STDOUT_FILENO, buffer + *i, 1) == -1) {
			perror("write failed");
		}
		(*i)++;
	}
}

// Por unifomidad se le pasa el buffer aunque no se use
void move_cursor_left(char *buffer __attribute__((unused)), size_t *i) {
	if (*i > 0) {
		if (write(STDOUT_FILENO, "\b", 1) == -1) {
			perror("write failed");
		}
		(*i)--;
	}
}

void move_cursor_right_word(char *buffer, size_t *i) {
	if (*i < strlen(buffer)) {
		// if there's a space, move cursor to the next word
		if (buffer[*i] == SPACE) {
			while (*i < strlen(buffer) && buffer[*i] == SPACE) {
				if (write(STDOUT_FILENO, buffer + *i, 1) == -1) {
					perror("write failed");
				}
				(*i)++;
			}
		}
		// move cursor to the end of the word
		while (*i < strlen(buffer) && buffer[*i] != SPACE) {
			if (write(STDOUT_FILENO, buffer + *i, 1) == -1) {
				perror("write failed");
			}
			(*i)++;
		}
	}
}

void move_cursor_left_word(char *buffer, size_t *i) {
	if (*i > 0) {
		// if there's a space, move cursor to the previous word
		if (buffer[*i - 1] == SPACE) {
			while (*i > 0 && buffer[*i - 1] == SPACE) {
				if (write(STDOUT_FILENO, "\b", 1) == -1) {
					perror("write failed");
				}
				(*i)--;
			}
		}

		// move cursor to the beginning of the word
		while (*i > 0 && buffer[*i - 1] != SPACE) {
			if (write(STDOUT_FILENO, "\b", 1) == -1) {
				perror("write failed");
			}
			(*i)--;
		}
	}
}

void empty_stdout(char *buffer, size_t *i) {
	// move cursor to the beginning of the line
    for (size_t j = 0; j < *i; j++) {
        if (write(STDOUT_FILENO, "\b", 1) == -1) {
			perror("write failed");
		}
    }

    // overwrite the entire line with spaces
    for (size_t j = 0; j < strlen(buffer); j++) {
        if (write(STDOUT_FILENO, " ", 1) == -1) {
			perror("write failed");
		}
    }
	
    // move cursor back to the beginning of the line
    for (size_t j = 0; j < strlen(buffer); j++) {
        if (write(STDOUT_FILENO, "\b", 1) == -1) {
			perror("write failed");
		}
    }
}

void empty_buffer(char *buffer) {
	for (size_t i = 0; i < BUFLEN; i++) {
		buffer[i] = END_STRING;
	}
}

void clean_before_exit(char **commands, int commands_count) {
	printf("\n");
	reset_input_mode();
	free_commands(commands, commands_count);
}

void free_commands(char **commands, int commands_count) {
	for (int i = 0; i < commands_count; i++) {
		free(commands[i]);
	}
	free(commands);
}