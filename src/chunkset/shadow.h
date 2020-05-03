
// TEMP FILE TEMP FILE

#ifndef SHADOW_H
#define SHADOW_H


/*
 * NEW API
 * shadow_init( set )
 * shadow_sample( xyz, normal )
 * shadow_place_update()
 *
 * stubs
 * shadow_break_update();
 *
 * 
 * Normal is 0 for X, 1 for Y
 *
 */


#define SH_MAX_X set->shadow_map_size[0]
#define SH_MAX_Z set->shadow_map_size[1]

void shadow_init(
	struct ChunkSet *set
){

	uint32_t wsy = (set->max[1] << set->root_bitw);

	set->shadow_map_size[0] = ((set->max[0] << set->root_bitw) + wsy );
	set->shadow_map_size[1] = (set->max[2] << set->root_bitw);

	set->shadow_map_length = 
		set->shadow_map_size[0] * 
		set->shadow_map_size[1];

	set->shadow_map = mem_alloc( 
		set->shadow_map_length *
		sizeof(uint16_t)
	);
}

uint32_t shadow_index(
	struct ChunkSet *set,
	uint32_t ws[3]
){
	//uint32_t max_x = set->shadow_map_size[0];
	return (ws[0] + ws[1]) + ( SH_MAX_X * ws[2] );
}

// returns 1 if in shadow
int shadow_sample( 
	struct ChunkSet *set,
	uint32_t *ws
){
	uint32_t index = shadow_index( set, ws );
	return !(
		set->shadow_map[ index ] < ws[1]+1 &&
		set->shadow_map[ index+1 ] < ws[1]+1
	);
}

int shadow_sample_normal( 
	struct ChunkSet *set,
	uint32_t *ws,
	uint32_t normal
){
	uint32_t index = shadow_index( set, ws );
	return !(
		set->shadow_map[ index   ] < ws[1]+1 &&
		set->shadow_map[ index-1 ] < ws[1]+1
	);
}

void shadow_place_update(
	struct ChunkSet *set,
	uint32_t *ws
){
	// If voxel shaded, dont do anything
	if( shadow_sample( set, ws ) ) return;

	uint32_t index = shadow_index( set, ws );
	set->shadow_map[ index   ] = ws[1];
	//set->shadow_map[ index+1 ] = ws[1];
	
	return;
}

void shadow_break_update(
	struct ChunkSet *set,
	uint32_t *ws
){
	// stub
	return;
}



/*
uint32_t shadow_map_index( 
	struct ChunkSet *set, 
	uint32_t x, 
	uint32_t y 
){
	#ifdef DISABLE_SHADOWS
	return 0;
	#endif
	return y * (set->max[0]<<set->root_bitw) + x;
}

uint8_t sample_shadow( 
	struct ChunkSet *set, 
	uint32_t *ws 
){
	#ifdef DISABLE_SHADOWS
	return 0;
	#endif
	return !
		(set->shadow_map[ shadow_map_index(
			set,
			ws[0]-ws[1],
			ws[2] //-ws[1]
		) & (set->shadow_map_length-1) ] < ws[1]+1)	;
}

uint8_t sample_shadow_c( 
	struct ChunkSet *set, 
	struct ChunkMD *c, 
	uint16_t *ws 
){
	#ifdef DISABLE_SHADOWS
	return 0;
	#endif
	return !
		(set->shadow_map[ shadow_map_index(
			set,
			(ws[0] + (c->offset[0]<<set->root_bitw))-ws[1],
			(ws[2] + (c->offset[2]<<set->root_bitw)) //-ws[1]
		) & (set->shadow_map_length-1) ] < ws[1]+1);
}

void shadow_place_update(
	struct ChunkSet *set,
	uint32_t *ws
){
	#ifdef DISABLE_SHADOWS
	return;
	#endif
	// If voxel shaded, dont do anything
	if( sample_shadow( set, ws ) ) return;
	//uint32_t wsy = (set->max[1] << set->root_bitw);
	uint32_t sx = ws[0]-(ws[1]);
	uint32_t sy = ws[2]; //-(ws[1]);
	set->shadow_map[ shadow_map_index(set, sx, sy) & (set->shadow_map_length-1) ] = ws[1];
}

void shadow_break_update(
	struct ChunkSet *set,
	uint32_t *ws
){
	#ifdef DISABLE_SHADOWS
	return;
	#endif
	// If voxel is the occluder, reflow
	if( sample_shadow( set, ws ) ) return;

//	uint32_t wsy = (set->max[1] << set->root_bitw);

	uint32_t sx = ws[0]-(ws[1]);
	uint32_t sy = ws[2]; //-(ws[1]);

	for (uint32_t y = ws[1]; y > 0; y--){

		uint32_t wsc[3];
		wsc[0] = sx+y;
		wsc[1] = y;
		wsc[2] = sy+y;

		if( chunkset_edit_read( set, wsc ) ){
			set->shadow_map[ shadow_map_index(set, sx, sy) & (set->shadow_map_length-1) ] = y;
			//printf("%i\n", ws[1] );
			return;
		}
	}
}

void shadow_update(
	struct ChunkSet *set,
	uint32_t *ws
){

	#ifdef DISABLE_SHADOWS
	return;
	#endif
	uint32_t wsy = (set->max[1] << set->root_bitw);

	uint32_t sx = ws[0]-(ws[1]);
	uint32_t sy = ws[2]; //-(ws[1]);

	for (uint32_t y = wsy; y > 0; y--){

		uint32_t wsc[3];
		wsc[0] = sx+y;
		wsc[1] = y;
		wsc[2] = sy+y;

		if( chunkset_edit_read( set, wsc ) ){
			set->shadow_map[ shadow_map_index(set, sx, sy) & (set->shadow_map_length-1) ] = y;
			//printf("%i\n", ws[1] );
			return;
		}
	}
}

void chunkset_create_shadow_map( 
	struct ChunkSet *set
 ){

	#ifdef DISABLE_SHADOWS
	return;
	#endif


	uint32_t wsy = (set->max[1] << set->root_bitw);

	set->shadow_map_size[0] = (set->max[0] << set->root_bitw)+wsy;
	set->shadow_map_size[1] = (set->max[2] << set->root_bitw)+wsy;

	set->shadow_map_length = 
		 (set->max[0]<<set->root_bitw)
		*(set->max[2]<<set->root_bitw);

	set->shadow_map = mem_alloc( 
		 set->shadow_map_size[0] * 
		 set->shadow_map_size[1] * 
		 sizeof(uint16_t)
	);
	for (int i = 0; i < set->shadow_map_size[0]*set->shadow_map_size[1]; ++i)
	{
		set->shadow_map[i] = 0;
	}

}

*/

#endif
