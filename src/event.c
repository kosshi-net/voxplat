#define __FILENAME__ "event.c"

#include "event.h"

#include "mem.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define MAX_EVENTS 128
#define MAX_BOUND_CALLBACKS 128

#define LOG_LINE_LENGTH 512

// Event system
// Also handles logging
// See header file for events.


#define LOG_LINES 255

//char*			log_history			[LOG_HISTORY_SIZE];
char * 			log_history;
uint8_t			log_history_level	[LOG_LINES];
uint8_t		 	log_last = 0;
unsigned int	log_length = 0;

unsigned int log_hist_length(){
	return log_length;
}

void log_init(){

	log_history = mem_alloc( LOG_LINE_LENGTH * LOG_LINES );

}

void log_history_push(Event level, const char*str){

	int s = 0; // start
	
	// Split the string on newlines and save them as separate

	for( int i = 0; 1; i++  ){
		
		if( !(str[i] == '\n' || str[i]=='\0') ){
			continue;
		}
		
		int len = i - s;

		if(len < 1) continue; // In case of eg. \n\0, dont push anything
		
		if( log_length > LOG_LINES ) 
			log_length = LOG_LINES;

		log_history_level[log_last] = level; 
		
		//strncpy( log_history[log_last], &str[s], len+1 );
		
		int _i = log_last * LOG_LINE_LENGTH; 

		memcpy( log_history+_i, &str[s], len+1 );
		log_history[_i+len] = '\0';

		log_last++;
		log_length++;

		s = i+1;

		if ( str[i]=='\0') break;	
	}
}

uint8_t log_hist_level(uint8_t n){
	uint8_t i = log_last-n;
	return log_history_level[ i ];
}

char* log_hist_line(uint8_t n){
	uint8_t i = log_last-n;
	
	return log_history + i*LOG_LINE_LENGTH ;

//	return log_history[ i ];
}

// These shouldn't be called directly
// Header file defines a bunch of stuff like logf_info
void _log_info( char*msg  ){
	log_history_push( EVENT_INFO,  msg );
	event_fire( EVENT_INFO, msg );
	printf( "%s\n", msg );
}

void _log_warn( char*msg ){
	log_history_push( EVENT_WARN, msg  );

	event_fire( EVENT_WARN, msg );
	printf( "%s\n", msg );

}

void _log_error( char*msg ){
	log_history_push( EVENT_ERROR, msg );

	event_fire( EVENT_ERROR, msg );
	printf( "%s\n", msg );
}

void (*callbacks[MAX_EVENTS][MAX_BOUND_CALLBACKS])(void*) = { 0 };


void event_init(){
	
}


void event_bind(Event e, void(*callback)(void*)){
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i] == NULL ) {
			callbacks[e][i] = callback;
			return;
		}
	}
}

void event_unbind(Event e, void(*callback)(void*)){
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i] == callback ) {
			callbacks[e][i] = NULL;
		}
	}
}

void event_fire(Event e, void*args){
	for (int i = 0; i < MAX_BOUND_CALLBACKS; ++i) {
		if( callbacks[e][i] != NULL ) callbacks[e][i](args);
	}
}


void panic(void)
{
	event_fire( EVENT_PANIC, NULL );
}
