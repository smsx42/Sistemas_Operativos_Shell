#ifndef READLINE_H
#define READLINE_H

void reset_input_mode(void);

void set_input_mode(void);

char *read_line(const char *prompt);

void insert_without_overwriting(char *buffer, size_t *i, int *c);

void backspace(char *buffer, size_t *i);

void escape_sequence(char *buffer, size_t *i, int *c, char **commands, int commands_count, size_t *current_command_index);

void previous_command(char *buffer, size_t *i, char **commands, int commands_count, size_t *current_command_index);

void next_command(char *buffer, size_t *i, char **commands, int commands_count, size_t *current_command_index);

void home(char *buffer, size_t *i);

void end(char *buffer, size_t *i);

void move_cursor_left(char *buffer, size_t *i);

void move_cursor_right(char *buffer, size_t *i);

void move_cursor_left_word(char *buffer, size_t *i);

void move_cursor_right_word(char *buffer, size_t *i);

void empty_buffer(char *buffer);

void empty_stdout(char *buffer, size_t *i);

void clean_before_exit(char **commands, int commands_count);

void free_commands(char **commands, int commands_count);

#endif  // READLINE_H
