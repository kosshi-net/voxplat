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
void chunk_make_splatlist( 
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	int16_t *geometry,
	uint32_t*geometry_items,
	uint8_t *mask
);

#endif
