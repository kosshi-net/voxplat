#define __FILENAME__ "gfx/shell.c"

#include "gfx/shell.h"

#include "gfx.h"
#include "gfx/text.h"
#include "ctx.h"
#include "event.h"
#include "../shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
void info_callback( void* _str ){


}
void gfx_text_init_ok_callback(){
	logf_info( "GFX_SHELL You should see something now." );
}
*/

int gfx_shell_init(){
	//event_bind( INFO, &info_callback );
	//event_bind( GFX_TEXT_INIT_OK, &gfx_text_init_ok_callback );
	gfx_shell_exclusive_draw();

	return 0;
}

void gfx_shell_exclusive_draw(){
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
	int width, height;
	ctx_get_window_size( &width, &height  );
			
	glViewport(0, 0, width, height);

	float sx = 2.0 / width;
	float sy = 2.0 / height;

	int font_size = FONT_SIZE;
	
	unsigned int log_length = log_hist_length();
	
	int max_rows = height / font_size;
	 
	uint8_t i = 0;
	
	if( log_length < max_rows )
		max_rows = log_length;

	int ln = 0;	
	int h = ( -font_size - 5 ) + height % font_size;
	

	float icolor[4] = { 1,1,1,1 };
	float wcolor[4] = { 1,1,0,1 };
	float ecolor[4] = { 1,0,0,1 };
	
	for(uint8_t i = 0; i < max_rows; i++ ){
		float*color;	
		Event level = log_hist_level(max_rows-i);
		switch (level){
			case EVENT_WARN:  color=wcolor; break;
			case EVENT_ERROR: color=ecolor; break;
			default: 	color=icolor; break;
		}
	
		h+=font_size;
		int nl = gfx_text_draw(
			log_hist_line(max_rows-i),
			-1 + 8 * sx, 
			 1 - h * sy, 
			sx, sy,
			color, font_size
		);
		
		ln += nl;
		h  += font_size * nl;
//		max_rows -= nl;

		ln++;
	}
	char tb[256];
	sprintf( tb, "> %s", shell_input_buffer() );

	h+=font_size;
	gfx_text_draw(
		tb,
		-1 + 8 * sx, 
		 1 - h * sy, 
		sx, sy,
		icolor, font_size
	);

	ctx_swap_buffers();
}

