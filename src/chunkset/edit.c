#define __FILENAME__ "chunkset/edit.h"

#include "chunkset.h"
#include "chunkset/edit.h"

#include <math.h>
#include <cglm/cglm.h>
#include "event.h"

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

	chunk_open_ro(set, c);
	Voxel v = c->voxels[ flatten1( voxel_vec, set->root_bitw ) ];;
	chunk_close_ro(set, c);
	
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

	chunk_open_rw(set, c);
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
	chunk_close_rw(set, c);
}


void chunkset_get_chunks_in_aabb(
	struct ChunkMD **clist, 
	struct ChunkSet *set, 
	int32_t* ws_min,
	int32_t* ws_max
){
	int16_t cs_min[3];
	int16_t cs_max[3];

	for (int i = 0; i < 3; ++i){
		cs_max[i] = ws_max[i] >> set->root_bitw;
		cs_min[i] = ws_min[i] >> set->root_bitw;
	}

	//logf_info("%i %i %i", cs_min[0], cs_min[1], cs_min[2]);
	//logf_info("%i %i %i", cs_max[0], cs_max[1], cs_max[2]);

	int16_t cs[3];

	int cc = 0;

	for (cs[0] = cs_min[0]; cs[0] <= cs_max[0]; ++cs[0])
	for (cs[1] = cs_min[1]; cs[1] <= cs_max[1]; ++cs[1])
	for (cs[2] = cs_min[2]; cs[2] <= cs_max[2]; ++cs[2])
	{
		for (int i = 0; i < 3; ++i)
			if( cs[i] < 0 || cs[i] >= set->max[i] ) goto out_of_bounds;

		//logf_info("Add %i %i %i", cs[0], cs[1], cs[2]);
	
		uint32_t ci = flatten3( (uint16_t*)cs, set->max_bitw );		
		clist[cc++] = set->chunks + ci;

		out_of_bounds:
		;
	}

}


int chunk_ws_inside(
	struct ChunkSet* set, 
	struct ChunkMD* c, 
	int32_t* ws
){
	for( int i = 0; i < 3; i++  ){
		if( ws[i]   < (c->offset[i] << set->root_bitw)
		||	ws[i]+1 > (c->offset[i] << set->root_bitw) + set->root
		) return 0;
	}
	return 1;
}


void chunk_ws_write(
	struct ChunkSet* set, 
	struct ChunkMD* c, 
	int32_t* ws, 
	Voxel voxel
){
	if (!chunk_ws_inside(set, c, ws)) return;

	if( ws[1] < 2 ) return;

	uint16_t voxel_vec[3];
	for( int i = 0; i < 3; i++  )
		voxel_vec[i] = ws[i] - ( c->offset[i] << set->root_bitw );

	if( !c ) {
		logf_error("bad chunk (null)");
		panic();
	}

	if( !c->voxels ) {
		logf_error(
			"No voxel data\n"
			"RLE%p\n"
			"OFFSET %i %i %i"
			,
			c->rle,
			c->offset[0], c->offset[1], c->offset[2]

		);
		panic();
	}

	c->voxels[ flatten1( voxel_vec, set->root_bitw ) ] = voxel; // I KEEP SEGFAULTING
}


void chunkset_edit_sphere(
	struct ChunkSet* set, 
	int32_t* ws, 
	uint32_t radius,
	Voxel voxel
){


	int32_t ws_min[3];
	int32_t ws_max[3];

	for (int i = 0; i < 3; ++i){
		ws_min[i] = ws[i]-radius-1;
		ws_max[i] = ws[i]+radius+1;
	}

	struct ChunkMD *list[1024] = {0};

	chunkset_get_chunks_in_aabb(
		list,
		set,
		ws_min,
		ws_max
	);

	struct ChunkMD **c = list;
	if(!*c) return;

	//do logf_info("%p", *c) while(*++c);

	int32_t u[3];

	c = list;
	do {
		chunk_open_rw(set, *c); 
		for( u[0] = ws_min[0]; u[0] < ws_max[0]; u[0]++ )
		for( u[1] = ws_min[1]; u[1] < ws_max[1]; u[1]++ )
		for( u[2] = ws_min[2]; u[2] < ws_max[2]; u[2]++ ){
			float _a[3], _b[3];
			for (int i = 0; i < 3; ++i){	
				_a[i] = (float)u[i];
				_b[i] = (float)ws[i];
			}
			if (glm_vec_distance(_a, _b) < radius){

				if(!chunk_ws_inside(set, *c, u)) continue;

				chunk_ws_write(set, *c, u, voxel); 
				//chunkset_edit_write(set, u, voxel);
				
				if(voxel)
					shadow_place_update(set, u);
				else{
					chunk_close_rw(set, *c); 
					shadow_break_update(set, u);
					chunk_open_rw(set, *c); 
				}
			}
		} 
		chunk_close_rw(set, *c); 
	} while(*++c);


	c = list;
	do (*c)->dirty = 1; while(*++c);

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
		deltaDist[i] = sqrt( 
			t[i][0]*t[i][0] + 
			t[i][1]*t[i][1] + 
			t[i][2]*t[i][2]
		);

		if (vector[i] < 0) {
			step[i] = -1.0f;
			next[i] = (origin[i] - coord[i]) * deltaDist[i];
		} else {
			step[i] = 1.0f;
			next[i] = (coord[i] + 1.0f - origin[i]) * deltaDist[i];
		}
	}

	int loops = 1024*4;
	
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
