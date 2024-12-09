#include "builtin.h"
#include "history.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char **get_history(char *history_file_path, int *commands_count)
{
	FILE *history_file = fopen(history_file_path, "a+");
	if (history_file == NULL) {
		perror("Error opening history file");
		*commands_count = -1;
		return NULL;
	}

	// Inicio el array de comandos como NULL, luego lo voy llenando.
	char **commands = NULL;

	char *line = NULL;
	size_t len = 0;
	int i = 0;
	while ((getline(&line, &len, history_file)) != -1) {
		commands = realloc(commands, (i + 1) * sizeof(char *));

		if (commands == NULL) {
			perror("Error reallocating memory");
			for (int j = 0; j < i; j++) {
				free(commands[j]);
			}
			free(commands);
			free(line);
			*commands_count = -1;
			return NULL;
		}

		commands[i] = strdup(line);

		if (commands[i] == NULL) {
			perror("Error allocating memory");
			for (int j = 0; j < i; j++) {
				free(commands[j]);
			}
			free(commands);
			free(line);
			*commands_count = -1;
			return NULL;
		}

		i++;
	}
	free(line);
	fclose(history_file);
	*commands_count = i;
	return commands;
}