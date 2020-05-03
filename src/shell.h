#ifndef	SHELL_H
#define	SHELL_H 1

#include <stdint.h>

void shell_bind_command(char* name, void (*callback)(int, char**));

void shell_push_char(char);
void shell_backspace(void);
void shell_send();
void shell_run(char*);
void shell_autocomplete();
char* shell_input_buffer();

size_t parse_long(char*);

#endif
