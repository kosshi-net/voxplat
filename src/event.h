#ifndef EVENT_EVENT_H
#define EVENT_EVENT_H 1

#include <stdio.h>
#include <stdint.h>

// logf_info
// works like printf


#ifndef __FILENAME__
#warning "Macro __FILENAME__ undefined!"
#define __FILENAME__ __FILE__
#endif

#define SS 512

char _str [SS];
char _strf[SS];

// improve me
#define logf_info(msg, ...) {\
	snprintf( _strf, SS, msg, ## __VA_ARGS__  );\
	log_info( _strf );}


#define log_info(msg) {\
	snprintf( _str, SS, "%s:%i :: %s", __FILENAME__, __LINE__, msg );\
	_log_info( _str  );}


#define logf_warn(msg, ...) {\
	snprintf( _strf, SS, msg, ## __VA_ARGS__  );\
	log_warn( _strf );}

#define log_warn(msg) {\
	snprintf( _str, SS, "%s:%i :: %s", __FILENAME__, __LINE__, msg );\
	_log_warn( _str  );}


#define logf_error(msg, ...) {\
	snprintf( _strf, SS, msg, ## __VA_ARGS__  );\
	log_error( _strf );}

#define log_error(msg) {\
	snprintf( _str, SS, "%s:%i :: %s", __FILENAME__, __LINE__, msg );\
	_log_error( _str  );}




unsigned int 	log_hist_length();
char* 			log_hist_line(uint8_t);
uint8_t 		log_hist_level(uint8_t);


#include "events.h"

void event_bind		(Event e, void(*callback)(void*));
void event_unbind	(Event e, void(*callback)(void*));
void event_fire		(Event e, void*args);

void log_init (void);

void _log_info	( char* );
void _log_warn	( char* );
void _log_error	( char* );

void panic(void);

#endif
