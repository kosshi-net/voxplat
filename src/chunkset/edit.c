#include "chunkset.h"
#include "chunkset/edit.h"

#include <math.h>

Voxel chunkset_edit_read(
	struct ChunkSet *set,
	uint32_t *ws
){
	uint16_t chunk_vec[3];
	uint16_t voxel_vec[3];
	
	for( int i = 0; i < 3; i++  ){
		chunk_vec[i] = ws[i] >> set->root_bitw;
		voxel_vec[i] = ws[i] - ( chunk_vec[i] << set->root_bitw );
	}

	if( ws[0] >= set->max[0]<<set->root_bitw
	||	ws[1] >= set->max[1]<<set->root_bitw
	||	ws[2] >= set->max[2]<<set->root_bitw
	 ) return 0;

	uint32_t ci = flatten3( chunk_vec, set->max_bitw );
	//if( ci > set->count ) return 0;
	struct ChunkMD *c = &set->chunks[ ci ];

	chunk_open_ro(c);
	Voxel v = c->voxels[ flatten1( voxel_vec, set->root_bitw ) ];;
	chunk_close_ro(c);
	
	return v;
}


void chunkset_edit_write(
	struct ChunkSet *set,
	uint32_t *ws,
	Voxel voxel
){
	uint16_t chunk_vec[3];
	uint16_t voxel_vec[3];
	
	for( int i = 0; i < 3; i++  ){
		chunk_vec[i] = ws[i] >> set->root_bitw;
		voxel_vec[i] = ws[i] - ( chunk_vec[i] << set->root_bitw );
	}

	if( ws[0] >= set->max[0]<<set->root_bitw
	||	ws[1] >= set->max[1]<<set->root_bitw
	||	ws[2] >= set->max[2]<<set->root_bitw
	 ) return;


	uint32_t ci = flatten3( chunk_vec, set->max_bitw );
	//if( ci > set->count ) return;
	struct ChunkMD *c = &set->chunks[ ci ];

	chunk_open_rw(c);
	Voxel *v = &c->voxels[ flatten1( voxel_vec, set->root_bitw ) ];
	if( *v != voxel  ) {
		*v = voxel;
		c->dirty = 1;

		// neighbors
		for (int i = 0; i < 3; ++i) {
			if( voxel_vec[i] == 0 ){
				chunk_vec[i]--;
				if( chunk_vec[i] < set->max[i] )
					set->chunks[flatten3( chunk_vec, set->max_bitw )].dirty = 1;
				chunk_vec[i]++;
			}
			if( voxel_vec[i] == set->root-1 ){
				chunk_vec[i]++;
				if( chunk_vec[i] < set->max[i] )
					set->chunks[flatten3( chunk_vec, set->max_bitw )].dirty = 1;
				chunk_vec[i]--;
			}
		}

	}
	chunk_close_rw(c);
}




Voxel chunkset_edit_raycast_until_solid(
	struct ChunkSet *set,
	float *origin,
	float *vector,
// Returns
	uint32_t *coord,
	int8_t *normal
){


	for (int i = 0; i < 3; ++i) {
		coord[i] = ((int)origin[i]);
	}

	float deltaDist[3];
	float next[3];
	float step[3];
	
	float t[3][3];

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 3; ++j) 
		t[i][j] = vector[j] / vector[i];


	for (int i = 0; i < 3; ++i) {
		deltaDist[i] = sqrt( t[i][0]*t[i][0] + t[i][1]*t[i][1] + t[i][2]*t[i][2] );

		if (vector[i] < 0) {
			step[i] = -1.0f;
			next[i] = (origin[i] - coord[i]) * deltaDist[i];
		} else {
			step[i] = 1.0f;
			next[i] = (coord[i] + 1.0f - origin[i]) * deltaDist[i];
		}
	}

	int loops = 1024;
	
	while (loops-- > 1) {
		int side = 0;

		for (int j = 1; j < 3; ++j) {
			if (next[side] > next[j]) {
				side = j;
			}
		}

		next[side] += deltaDist[side];
		coord[side] += step[side];
		
		Voxel v = chunkset_edit_read( set, coord );

		if( v > 0 ) {
			normal[side] = (-vector[side] > 0) ? +1 : -1;

			return v;
		}

	}
	return 0;

}
