#ifndef GFX_VMESH_H
#define GFX_VMESH_H 1

#include <cglm/cglm.h>
#include <stdlib.h>
#include "chunkset.h"
#include "gfx/camera.h"

int  gfx_vmesh_init(void);

void gfx_vmesh_draw( 
	struct Camera *cam,
	struct ChunkSet *set,
	struct GeometryMesh **queue,
	uint32_t queue_count,
	uint32_t *item_count
);


void gfx_update_mesh( 
	struct ChunkSet *set,
	uint32_t index
);


void gfx_delete_mesh( 
	struct ChunkSet *set,
	uint32_t index
);

#endif