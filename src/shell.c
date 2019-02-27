#define __FILENAME__ "shell.c"

#include <string.h>
#include "event.h"

#include "gfx/shell.h"
#include "ctx.h"
#include <unistd.h>

#include "mem.h"

// CLI User input processing and commands
// Render code found in gfx/shell.c
//
// These functions are mostly called from ctx/input.c 

struct Command{
	char name[64];
	void (*run)(int, char**);
};

// COMMAND FUNCTIONS
void command_echo(int argc, char** argv){
	for(int i = 1; i < argc; i++){
		logf_info("%s", argv[i]);
	}
}
void command_ayy(){
	logf_info("lmao");
}

void command_exit(){
    event_fire(EVENT_EXIT, NULL);
	log_info("Goodbye!");
	gfx_shell_exclusive_draw();
	sleep( 1 );
	ctx_terminate();
	exit(0);
}
void command_help();
// Map them
int command_count = 4;
struct Command commands [64] = {
	{ .name = "echo", .run = &command_echo },
	{ .name = "help", .run = &command_help },
	{ .name = "ayy", .run = &command_ayy },
	{ .name = "exit", .run = &command_exit }
};

void command_help(){
	for( int i = 0; i < command_count; i++ ){
		logf_info("%s", commands[i].name);
	}
}


void shell_bind_command(char* name, void (*callback)(int, char**)){
	memcpy( commands[command_count].name, name, strlen(name)+1 );
	commands[command_count].run = callback;

	command_count++;
}


char shell_inbuf[128] = { 0 };
int  shell_inbuf_len = 0;

void shell_push_char(char c){
	// no buffer overflow protection )=
	shell_inbuf[shell_inbuf_len] = c;
	shell_inbuf_len++;
}
void shell_backspace(){
	if(shell_inbuf_len==0) return;
	shell_inbuf_len--;
	shell_inbuf[shell_inbuf_len] = 0;
}

int prefix(const char *pre, const char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}
void shell_autocomplete(){
	char *candidate = NULL;
	for( int i = 0; i < command_count; i++ ){
		if( prefix( shell_inbuf, commands[i].name ) ){
			if( candidate ) return;
			candidate = commands[i].name;
		}
	}
	if( !candidate ) return;
	memcpy( shell_inbuf, candidate, strlen(candidate));
	shell_inbuf_len = strlen(candidate);
}

void shell_send(){
	// Print command
	char tb[256];
	sprintf( tb, "> %s", shell_inbuf );
	_log_info( tb );

	// Alloc argv
	char* argv[32];
	for( int i = 0; i < 32; i++ )
		argv[i] = mem_calloc(64);

	// Split and fill argv
	int argc = 0;
	int j = 0;
	char *p = shell_inbuf;
	while( *p ){
		if( *p == ' ' ){
			argc++;
			j = 0;
		}else{
			argv[argc][j++] = *p;
		}
		p++;
	}

	shell_inbuf_len = 0;
	memset( shell_inbuf, 0, sizeof(shell_inbuf) );

	// Find and execute command
	for( int i = 0; i < command_count; i++ ){
		if( strcmp(argv[0], commands[i].name) == 0 ){
			commands[i].run( argc+1, argv );
			goto exit;
		}
	}
	logf_warn("Command not found");
	exit:

	// Cleanup
	for( int i = 0; i < 32; i++ )
		mem_free(argv[i]);

}

char* shell_input_buffer(){
	return shell_inbuf;
}