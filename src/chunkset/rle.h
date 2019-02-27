#ifndef RLE_H
#define RLE_H 1

#include <stdint.h>
#include "chunkset.h"

Voxel *rle_compress( Voxel*, uint32_t alength );
Voxel *rle_decompress( Voxel* );

#endif
