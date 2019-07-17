#ifndef GFX_VSPLAT_H
#define GFX_VSPLAT_H 1

#include <cglm/cglm.h>
#include <stdlib.h>
#include "chunkset.h"
#include "gfx/camera.h"

int  gfx_vsplat_init(void);

void gfx_vsplat_draw( 
	struct Camera *cam,
	struct ChunkSet *set,
	struct ChunkMD **queue,
	uint32_t queue_count,
	uint32_t *item_count
);
	


void gfx_update_svl( 
	struct ChunkSet *set,
	uint32_t index
);


void gfx_svl_draw( 
	struct Camera *cam,
	struct ChunkSet *set,
	struct GeometrySVL **queue,
	uint32_t queue_count,
	uint32_t *item_count
);


#endif