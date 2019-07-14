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



int chunk_ws_inside(
	struct ChunkSet* set, 
	struct ChunkMD* c, 
	int32_t* ws
);


void chunk_ws_write(
	struct ChunkSet* set, 
	struct ChunkMD* c, 
	int32_t* ws,
	Voxel voxel
);

void chunkset_edit_sphere(
	struct ChunkSet* set, 
	int32_t* ws, 
	uint32_t radius,
	Voxel voxel
);


#endif
