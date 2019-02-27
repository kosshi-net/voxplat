#ifndef EVENTS_H
#define EVENTS_H 1


// Define every event in the whole project here.
// This file will be included with event.h, so no need to include this as well.


typedef enum {

	// Logging events.
	EVENT_INFO,
	EVENT_WARN,
	EVENT_ERROR,
	
	EVENT_LOCKING_PROGRESS,

	// This casuses the program to exit almost immediatetly
	EVENT_PANIC,	

	EVENT_EXIT 

} Event;

#endif
