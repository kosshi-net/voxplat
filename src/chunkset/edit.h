#ifndef WORLDEDIT_H
#define WORLDEDIT_H 1

#include "chunkset.h"

Voxel chunkset_edit_read( struct ChunkSet*, uint32_t*  );
void 	chunkset_edit_write( struct ChunkSet*, uint32_t*, Voxel );

Voxel chunkset_edit_raycast_until_solid(
	struct ChunkSet*, 
	float *origin, 
	float *vector, 
	uint32_t *hitcoord,
	int8_t *normal
);

#endif
