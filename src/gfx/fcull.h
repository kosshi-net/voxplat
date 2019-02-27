#ifndef	GFX_FCULL_H
#define	GFX_FCULL_H 1

#include <stdlib.h>

size_t gfx_fcull_planes_size();

void gfx_fcull_extract_planes(float* M, float*planes);

int gfx_fcull(float*planes, float*loc, float radius);

#endif