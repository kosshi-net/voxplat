#define __FILENAME__ "ctx/input.c"

#define CTX_H_INCLUDE_GLFW 1
#include "ctx.h"
#include "ctx/input.h"

#include <string.h>

#include "event.h"

#include "shell.h"

GLFWwindow *window;


InputMode input_mode = INPUT_MODE_NONE;
InputMode preferred_input_mode = INPUT_MODE_NONE;

void ctx_input_set_mode( InputMode mode ){
	input_mode = mode;

	switch (input_mode) {
		case INPUT_MODE_NONE:
			ctx_unlock_cursor();
			break;
		case INPUT_MODE_GAME:
			ctx_lock_cursor();
			break;
		case INPUT_MODE_SHELL:
			ctx_unlock_cursor();
			break;
	}
}

InputMode	ctx_input_preferred_mode(){
	return preferred_input_mode;
}

InputMode	ctx_input_mode(){
	return input_mode;
}



int break_event = 0;
int place_event = 0;

void mouse_key_callback( GLFWwindow* window, 
	int button, int action, int mods  
) {

	switch (input_mode) {

		case INPUT_MODE_NONE:

			if(button == GLFW_MOUSE_BUTTON_LEFT
			&& action == GLFW_RELEASE) 
				preferred_input_mode = INPUT_MODE_GAME;
			break;


		case INPUT_MODE_GAME:

			if(button == GLFW_MOUSE_BUTTON_LEFT 
			&& action == GLFW_PRESS) break_event++;
			
			if(button == GLFW_MOUSE_BUTTON_RIGHT 
			&& action == GLFW_PRESS) place_event++;
			break;


		case INPUT_MODE_SHELL:
			break;
	}

}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (input_mode) {
		case INPUT_MODE_NONE:
			if(key == GLFW_KEY_F12
			&& action == GLFW_RELEASE)
				preferred_input_mode = INPUT_MODE_SHELL;
			break;

		case INPUT_MODE_GAME:


			if(key == GLFW_KEY_F12
			&& action == GLFW_RELEASE)
				preferred_input_mode = INPUT_MODE_SHELL;

			if(key == GLFW_KEY_ESCAPE
			&& action == GLFW_RELEASE)
				preferred_input_mode = INPUT_MODE_NONE;
			break;

		case INPUT_MODE_SHELL:
			if(key == GLFW_KEY_F12
			&& action == GLFW_RELEASE)
				preferred_input_mode = INPUT_MODE_GAME;


			if(key == GLFW_KEY_BACKSPACE
			&& action == GLFW_PRESS) shell_backspace();

			if(key == GLFW_KEY_ENTER
			&& action == GLFW_PRESS) shell_send();	

			if(key == GLFW_KEY_TAB
			&& action == GLFW_PRESS) shell_autocomplete();	

			if(key == GLFW_KEY_ESCAPE
			&& action == GLFW_RELEASE)
				preferred_input_mode = INPUT_MODE_NONE;
			break;
	}
}


void character_callback(GLFWwindow* window, unsigned int codepoint)
{

	switch (input_mode) {
		case INPUT_MODE_NONE:
			break;

		case INPUT_MODE_GAME:
			break;

		case INPUT_MODE_SHELL:
			shell_push_char((char)codepoint);
			break;
	}
}

int ctx_input_init(){
	window = ctx_get_window();

	glfwSetMouseButtonCallback( window, mouse_key_callback );
	glfwSetKeyCallback( 		window, key_callback );
	glfwSetCharCallback( 		window, character_callback);

	logf_info( "--------- CONTROLS ---------" );
	logf_info( "Click to lock cursor" );
	logf_info( "ESC to unlock cursor " );
	logf_info( "LMB to break voxels" );
	logf_info( "RMB to place voxels" );
	logf_info( "WASD to move around" );
	logf_info( "F12 to toggle shell" );
	logf_info( "----------------------------" );
	logf_info("Init ok");

	return 0;
}

int ctx_input_break_block()
{
	//return ( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)== GLFW_PRESS );
	if( break_event ) {
		break_event = 0;
		return 1;
	}
	return 0;
}

int ctx_input_place_block()
{
//	return ( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)== GLFW_PRESS );
	if( place_event ) {
		place_event = 0;
		return 1;
	}
	return 0;
}




void ctx_movement_vec( int*x, int*y ){
	if(input_mode != INPUT_MODE_GAME) {
		*x = 0;
		*y = 0;
		return;
	}
	
	*x +=  1.0*(glfwGetKey(window, GLFW_KEY_W)==GLFW_PRESS);
	*x += -1.0*(glfwGetKey(window, GLFW_KEY_S)==GLFW_PRESS);

	*y +=  1.0*(glfwGetKey(window, GLFW_KEY_A)==GLFW_PRESS);
	*y += -1.0*(glfwGetKey(window, GLFW_KEY_D)==GLFW_PRESS);

}

int cursor_locked;

int ctx_cursor_locked(void){
	return cursor_locked;
}
void ctx_lock_cursor(void){
	glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED  );
	cursor_locked = 1;
}
void ctx_unlock_cursor(void){
	glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_NORMAL  );
	cursor_locked = 0;
}
void ctx_cursor_pos( double*x, double*y  ){
	glfwGetCursorPos(window, x, y);
}

double cursor_last_x_pos = 0;
double cursor_last_y_pos = 0;

void ctx_cursor_movement( double*x, double*y  ){
	double new_x, new_y;
	ctx_cursor_pos(&new_x, &new_y);

	if( !cursor_locked  ) {
		*x = 0;
		*y = 0;
	} else {
		*x = cursor_last_x_pos - new_x;
		*y = cursor_last_y_pos - new_y;
	}

	cursor_last_x_pos = new_x;
	cursor_last_y_pos = new_y;
}