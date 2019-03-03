#ifndef MESHER_H
#define MESHER_H 1


#include "chunkset.h"

void chunk_make_mesh( 
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	int16_t *geometry,
	uint32_t*geometry_items,
	uint32_t*index,
	uint32_t*index_items
);


void chunk_make_mask( 
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	uint8_t *mask
);

void chunk_mask_downsample( 
	struct ChunkSet *setp,
	int8_t	lod,
	Voxel *mask,
	Voxel *work
);

void chunk_make_splatlist( 
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	uint8_t level,
	Voxel *mask,
	int16_t *geometry,
	uint32_t*geometry_items
);

#endif
