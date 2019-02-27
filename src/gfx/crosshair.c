#define __FILENAME__ "gfx/crosshair.c"
#include "gfx/crosshair.h"

#include "ctx.h"

#include "gfx.h"
#include "gfx/rect.h"

#include <stdlib.h>


void gfx_crosshair_draw() {

	float min[2];
	float max[2];
	float color[4] = { 1.0, 1.0, 1.0, 0.8 };

	int wx, wy;
	ctx_get_window_size( &wy, &wx );

	// TODO
	// Make this more efficient and pixel-perfect

	float gap = 5.0;
	float thick = 1.0;
	float length = 10.0;

	min[1] = (  (wx/2.0) + gap           )/wx * 2.0f - 1.0f;
	min[0] = (  (wy/2.0) + thick         )/wy * 2.0f - 1.0f;
	max[1] = (  (wx/2.0) + gap + length  )/wx * 2.0f - 1.0f;
	max[0] = (  (wy/2.0) - thick         )/wy * 2.0f - 1.0f;

	gfx_rect_draw(min, max, color);

	min[1] = -min[1];
	max[1] = -max[1];

	gfx_rect_draw(min, max, color);

	min[1] = (  (wx/2.0) + thick         )/wx * 2.0f - 1.0f;
	min[0] = (  (wy/2.0) + gap           )/wy * 2.0f - 1.0f;
	max[1] = (  (wx/2.0) - thick 		 )/wx * 2.0f - 1.0f;
	max[0] = (  (wy/2.0) + gap + length  )/wy * 2.0f - 1.0f;

	gfx_rect_draw(min, max, color);

	min[0] = -min[0];
	max[0] = -max[0];

	gfx_rect_draw(min, max, color);



	return;
};
	