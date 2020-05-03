#define __FILENAME__ "main.c"
#include <config.h>

#include "ctx.h"
#include "cfg.h"
#include "ctx/input.h"
#include "gfx.h"
#include "gfx/text.h"
#include "gfx/shell.h"
#include "res.h"
#include "event.h"
#include "mem.h"
#include "cpp/noise.h"

#include "game.h"

#include <stdio.h>
#include <string.h>

#include <glad/glad.h> // OpenGL
#include <cglm/cglm.h>

#include <threadpool.h>
#include <pthread.h>

#include <unistd.h>

#include <time.h>

void log_compiler_info();

int errors = 0;

void error_callback(void* ptr)
{
	errors++;
}


void panic_callback(void* ptr)
{
	logf_error( "Unrecoverable error has happened" );
	while( !ctx_should_close() ){
		gfx_shell_exclusive_draw();
		ctx_poll_events();
		sleep(1);
	}
	exit(1);
}


pthread_mutex_t mutex_prog = PTHREAD_MUTEX_INITIALIZER;

void locking_progress_callback(void* ptr)
{
	pthread_mutex_lock( &mutex_prog  );

	ctx_poll_events();
	gfx_shell_exclusive_draw();

	if( ctx_should_close() ) {
		logf_warn("Uncleanly exiting via locking_progress_callback");
    	event_fire(EVENT_EXIT, NULL);
		gfx_shell_exclusive_draw();
		
		exit(0);
	}

	pthread_mutex_unlock( &mutex_prog  );
}


void check_glerr(void){
	GLenum err;
	if ((err = glGetError()) != GL_NO_ERROR){
		logf_error("OpenGL Error %i", err);
		panic();
	}
}

int main(int argc, char **argv)
{	
	cfg_init(argc, argv);

	mem_init( cfg_get()->heap );

	log_init();

	
	event_bind( EVENT_ERROR, error_callback  );
	event_bind( EVENT_PANIC, panic_callback  );
	event_bind( EVENT_LOCKING_PROGRESS, locking_progress_callback  );

	logf_info( "-----------" );
	logf_info( "%s %s.%s", 
		PROJECT_NAME,
		VERSION_MAJOR,
		VERSION_MINOR	
	);
	logf_info( "Copyright 2020 kosshi.net" );
	
	log_compiler_info();

	srand(time(0));

	if( ctx_init() ) return 1;	
	if( gfx_init() ) return 1;

	check_glerr();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	ctx_input_init();
	gfx_shell_exclusive_draw();
	check_glerr();	

	noise_init();

	logf_info("Init game ...");
	
	if( game_init() ) return 1;
	check_glerr();
	
	logf_info("Init done!");

	game_tick();
	ctx_poll_events();

	threadpool_init();

    while ( !ctx_should_close() )
    {	
    
    	check_glerr();

    	ctx_input_set_mode( ctx_input_preferred_mode() );

		if(errors || ctx_input_mode() == INPUT_MODE_SHELL )	
			gfx_shell_exclusive_draw();
		else
			game_tick();
	
		ctx_poll_events();
    }
    event_fire(EVENT_EXIT, NULL);
	log_info("Goodbye!");
	gfx_shell_exclusive_draw();
	ctx_terminate();
	exit(0);
}


void log_compiler_info(){
	#if __MINGW32__
	logf_info("Compiled with MinGW %i.%i",
		__MINGW32_MAJOR_VERSION,
		__MINGW32_MINOR_VERSION
	);
	logf_info("Switch to GNU/Linux - your freedom depends on it.");

	#elif __GNUC__

	logf_info("Compiled with GCC %i.%i", 
		__GNUC__, 
		__GNUC_MINOR__
	);

	#else

	logf_info("Unknown compiler");

	#endif

	logf_info("STDC: %li", __STDC_VERSION__);
	logf_info("Date: %s", __DATE__);
	logf_info("-----------------");
}
