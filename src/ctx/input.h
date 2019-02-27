#ifndef CTX_INPUT_H
#define CTX_INPUT_H 1

/*
 * This module is spaghetti code for managing input
 * Before rendering or running game code, decide what mode gets focus on that
 * frame. All game logic can still call their functions, but only the focused
 * mode gets input updates. Allow stuff like console rendering on top of game,
 * while only the console updates on input
 *
 *
 *
 * INPUT MODES
 * - GAME
 * - SHELL
 *
 *
*/

typedef enum {
	INPUT_MODE_NONE,
	INPUT_MODE_GAME,
	INPUT_MODE_SHELL
} InputMode;


int ctx_input_init(void);

void		ctx_input_set_mode( InputMode );
InputMode	ctx_input_mode( void );
InputMode 	ctx_input_preferred_mode( void );


void ctx_enable_cursor_controller(void);

int ctx_input_break_block(void);
int ctx_input_place_block(void);
void ctx_movement_vec( int*, int* );


void ctx_input_poll				(void);

int ctx_cursor_locked(void);
void ctx_lock_cursor(void);
void ctx_unlock_cursor(void);
void ctx_cursor_movement( double*x, double*y  );

#endif
