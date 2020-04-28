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

#define SHELL_INPUT_SIZE 128


size_t parse_long(char *c){
	size_t num = 0;
	while(*c){
		switch(*c){
			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7': case '8':
			case '9':
				num = num * 10 + (*c++ - '0');
				break;
			case 'G':
			case 'g':
				num *= 1024;
			case 'M':
			case 'm':
				num *= 1024;
			case 'K':
			case 'k':
				num *= 1024;
				c++;
				goto look_for_null;
			default:
				return 0;
				break;
		}
	}
	look_for_null:
	if( *c == '\0' ) return num;
	return 0;
}


struct Command{
	char name[64];
	void (*run)(int, char**);
};

// COMMAND FUNCTIONS
void command_echo(int argc, char** argv){
	for(int i = 1; i < argc; i++){
		_log_info(argv[i]);
	}
}
void command_ayy(){
	_log_info("lmao");
}

void command_clear(){
	log_clear();
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
int command_count = 5; // <------------------- !!!
struct Command commands [64] = {
	{ .name = "echo", .run = &command_echo },
	{ .name = "help", .run = &command_help },
	{ .name = "clear", .run = &command_clear },
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


char shell_inbuf[SHELL_INPUT_SIZE] = { 0 };
int  shell_inbuf_len = 0;

void shell_push_char(char c){
	if( shell_inbuf_len > SHELL_INPUT_SIZE-2 ) return;
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
	int suggest = 0;
	for( int i = 0; i < command_count; i++ ){
		if( prefix( shell_inbuf, commands[i].name ) ){
			if( candidate ) {
				if(!suggest) {
					log_info( candidate );
					suggest = 1;
				}
				log_info( commands[i].name );
			}
			candidate = commands[i].name;
			
		}
	}
	if( !candidate || suggest ) return;
	memcpy( shell_inbuf, candidate, strlen(candidate));
	shell_inbuf_len = strlen(candidate);
}

void shell_send(){
	// Print command
	char tb[SHELL_INPUT_SIZE+2];
	sprintf( tb, "> %s", shell_inbuf );
	_log_info( tb );
	
	// Split and fill argv
	char *argv[SHELL_INPUT_SIZE/2];
	int32_t size = SHELL_INPUT_SIZE; 
	argv[1] = argv[0] = mem_alloc( size );
	uint8_t argc = 1;

	char *p = shell_inbuf;
	do 
		if( *p == ' ' ) {
			*argv[argc++]++ = 0;
			argv[argc] = argv[argc-1];
		} else 
			*argv[argc]++ = *p;
	while( *p++ && --size );

	shell_inbuf_len = 0;
	memset( shell_inbuf, 0, sizeof(shell_inbuf) );
	for( int i = 0; i < command_count; i++ ){ 
		if( strcmp(argv[0], commands[i].name) == 0 ){
			commands[i].run( argc, argv );
			goto exit;
		}
	}
	logf_warn("Command not found");
	exit:
	
	mem_free(argv[0]);

}

char* shell_input_buffer(){
	return shell_inbuf;
}
