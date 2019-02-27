#ifndef GFX_AABB_H
#define GFX_AABB_H 1

#include "gfx/camera.h"

int  gfx_aabb_init(void);
void gfx_aabb_push( float *min3, float *max3 );
void gfx_aabb_draw( struct Camera *cam, float *color );
					
#endif