#define __FILENAME__ "gen.c"

#include "chunkset.h"
#include "chunkset/edit.h"
#include "cpp/noise.h"

#include "mem.h"
#include "event.h"
#include "math.h"
#include "ctx.h"

#include <string.h>
#include <omp.h>

uint32_t trees = 0;

void chunkset_gen_tree( struct ChunkSet* set, uint32_t x, uint32_t z ){

	uint32_t ws[] = {x,0,z};

	// Loop from 0 and find air;
	for( int i = 0; i < 200; i++ ){
		ws[1] = i;
		if(chunkset_edit_read( set, ws ) == 0 ) break;
	}
	if( ws[1] > 100 ) return;
	if( ws[1] < 10 ) return;
	
	trees++;

	uint32_t ws2[3];

	memcpy( ws2, ws, 3*sizeof(uint32_t));
	ws2[0] += 3; ws2[1] += 5; ws2[2] += 3;
	for( int i = 0; i < 3; i++ ){
		for( int x = 0; x < 7-i*2; x++ )
		for( int y = 0; y < 2; y++ )
		for( int z = 0; z < 7-i*2; z++ ){
			if( noise_randf() > 0.5 ) continue;
			uint32_t ws3[3];
			memcpy( ws3, ws2, 3*sizeof(uint32_t));
			ws3[0] -= x; ws3[1] -= y; ws3[2] -= z;
			chunkset_edit_write( set, ws3, 4 );
			shadow_place_update(set, ws3);
		}
		//memcpy( ws2, ws, 3*sizeof(uint32_t));
		ws2[0] -= 1; ws2[1] += 4; ws2[2] -= 1;
	}

	// Trunk
	for( int i = 0; i < 14; i++ ){
		chunkset_edit_write( set, ws, 36 );
		ws[1]++;
	}
	chunkset_edit_write( set, ws, 4 );
	shadow_place_update(set, ws);
	ws[1]++;
	chunkset_edit_write( set, ws, 4 );
	shadow_place_update(set, ws);

}
// DEPRICATED
void chunkset_gen_trees( struct ChunkSet* set){

	logf_info("Generating trees ..");
	event_fire( EVENT_LOCKING_PROGRESS, NULL );
	for( int x = 64; x < set->max[0]*set->root - 64; x+=10 ){
	/*	if( x%32 == 0 ){
			logf_info("Compressing");
			event_fire( EVENT_LOCKING_PROGRESS, NULL );
			chunkset_force_compress(set);
		}*/
	#pragma omp parallel for
	for( int z = 64; z < set->max[2]*set->root - 64; z+=10 ){

		float hz = 0.5;
		if( noise_simplex( x*hz, 0, 	z*hz ) > noise_randf() ) continue;
		hz = 10.0;
		float var_x = noise_simplex( x*hz, 0, 	z*hz );
		float var_y = noise_simplex( x*hz, 128, 	z*hz );
		chunkset_gen_tree( set, x + 5*var_x, z + 5*var_y );
		
	}}
	logf_info("%i trees planted", trees);
}


float get_height(struct ChunkSet *set, uint32_t x, uint32_t z){

	float hz = 0.005;

	float a0 = 1.0 - pow( noise_u_simplex_f3( x*hz, 0, z*hz ) , 2);

	hz = 0.04;
	float e0 = 
		(1.0f/1.0f) * 
		(1-fabs(noise_simplex( x*hz, 0, z*hz )));

	hz = hz*2;
	float e1 = 
		(1.0f/2.0f) * 
		(1-fabs(noise_simplex( x*hz, 0, z*hz))) *
		(e0);

	hz = hz*2;
	float e2 = 
		(1.0f/3.0f) * 
		(1-fabs(noise_simplex( x*hz, 0, z*hz))) *
		(e0+e1);

	hz = hz*2;
	float e3 = 
		(1.0f/4.0f) * 
		(1-fabs(noise_simplex( x*hz, 0, z*hz))) *
		(e0+e1+e2);

	float h = ((pow((e0+e1+e2+e3)*a0, 1.2) )*130.0f);

	hz = 5.0;
	h += 3 * noise_simplex( x*hz, 0, z*hz );
	hz = 10.0;
	h += 2 * noise_simplex( x*hz, 0, z*hz );


	float centre = set->max[0] * set->root / 2.0f;

	float edge = 1.0 - fmin(fabs( x - centre )/centre, 1.0);
	edge = fmin(1.0 - fmin(fabs( z - centre )/centre, 1.0), edge);

	edge*=5;
	edge = fmin(edge, 1.0);

	h*=edge;

	// Water level
	h -= 80.0f;
	h = fmax(h, 8.0f);

	return h;
}

#define FEATMAP_TREE_BIT 16


void chunkset_edit_raw_write(
	struct ChunkSet* set, 
	struct ChunkMD* c, 
	uint32_t* ws, 
	Voxel voxel
){
	//uint16_t chunk_vec[3];

	// Check if ws in chunk
	for( int i = 0; i < 3; i++  ){
		if( ws[i]   < (c->offset[i] << set->root_bitw)
		||	ws[i]+1 > (c->offset[i] << set->root_bitw) + set->root
		) return;
	}

	uint16_t voxel_vec[3];
	
	for( int i = 0; i < 3; i++  ){
		//chunk_vec[i] = ws[i] >> set->root_bitw;
		voxel_vec[i] = ws[i] - ( c->offset[i] << set->root_bitw );
	}

	Voxel *v = &c->voxels[ flatten1( voxel_vec, set->root_bitw ) ];
	*v = voxel;

}
void chunkset_gen_tree_in_chunk(
	struct ChunkSet* set, 
	struct ChunkMD* c, 

	uint32_t* ws
){

	uint32_t ws2[3];

	memcpy( ws2, ws, 3*sizeof(uint32_t));
	ws2[0] += 3; ws2[1] += 5; ws2[2] += 3;
	for( int i = 0; i < 3; i++ ){
		for( int x = 0; x < 7-i*2; x++ )
		for( int y = 0; y < 2; y++ )
		for( int z = 0; z < 7-i*2; z++ ){
			if( noise_randf() > 0.5 ) continue;
			uint32_t ws3[3];
			memcpy( ws3, ws2, 3*sizeof(uint32_t));
			ws3[0] -= x; ws3[1] -= y; ws3[2] -= z;
			chunkset_edit_raw_write( set, c, ws3, 4 );
			shadow_place_update(set, ws3);
		}
		//memcpy( ws2, ws, 3*sizeof(uint32_t));
		ws2[0] -= 1; ws2[1] += 4; ws2[2] -= 1;
	}

	// Trunk
	for( int i = 0; i < 14; i++ ){
		chunkset_edit_raw_write( set, c, ws, 36 );
		ws[1]++;
	}
	chunkset_edit_raw_write( set, c, ws, 4 );
	shadow_place_update(set, ws);
	ws[1]++;
	chunkset_edit_raw_write( set, c, ws, 4 );
	shadow_place_update(set, ws);

}


void chunkset_gen( struct ChunkSet* set )
{
	//float seed = noise_randf()*2048+1212;
	
	uint32_t done = 0;


	uint32_t ws_max_x = set->max[0]*set->root;
	uint32_t ws_max_z = set->max[2]*set->root;

	uint32_t *featuremap = mem_alloc(ws_max_x * ws_max_z * sizeof(uint32_t));

	#pragma omp parallel for
	for (int x = 0; x < ws_max_x ; ++x)
	for (int z = 0; z < ws_max_z ; ++z)
	{
		featuremap[ x | z * ws_max_x ] = get_height(set, x, z);
	}
	uint32_t trees = 0;
	#pragma omp parallel for
	for( int x = 64; x < ws_max_x - 64; x+=10 )
	for( int z = 64; z < ws_max_z - 64; z+=10 ){

		float hz = 0.5;
		if( noise_simplex( x*hz, 0, z*hz ) > noise_randf() ) continue;
		hz = 10.0;
		float var_x = noise_simplex( x*hz, 0, 	z*hz );
		float var_z = noise_simplex( x*hz, 128, 	z*hz );
		//chunkset_gen_tree( set, x + 5*var_x, z + 5*var_y );

		uint32_t feat_index = (uint32_t)(x+5*var_x) | (uint32_t)(z+5*var_z) * ws_max_x;

		uint32_t y = featuremap[ feat_index ];

		if( y > 10 && y < 100 ){
			featuremap[ feat_index ] = featuremap[ feat_index ] | (1 << FEATMAP_TREE_BIT);

			#pragma omp atomic
			trees++;
		}
		
	}
	logf_info("%i trees generated", trees);


	double last_report = ctx_time()-10.0f;

	#pragma omp parallel for
	for( uint32_t i = 0; i < set->count; i++ ){
		
		if( omp_get_thread_num() == 0 && last_report+1.0f < ctx_time()){
			last_report = ctx_time();
			logf_info( 
				"Generating .. %.0f%% (%li MB)", 
				done / (float)set->count * 100.0f,
				mem_dynamic_allocation() / 1024 / 1024 
			);
			
			event_fire( EVENT_LOCKING_PROGRESS, NULL );
		}
		

		struct ChunkMD *c = &set->chunks[i];
		chunk_open_rw(set, c);

		int32_t T = 2;

		int32_t svec[3] = {0};

		for( svec[0] = -T; svec[0] < set->root+T; svec[0]++  )
		for( svec[2] = -T; svec[2] < set->root+T; svec[2]++  )
		{

			uint32_t ws[3];
			ws[0] = set->root * c->offset[0] + svec[0];
			ws[2] = set->root * c->offset[2] + svec[2];
	
			if( set->root * c->offset[0] + svec[0] >= ws_max_x
			||  set->root * c->offset[2] + svec[2] >= ws_max_z
			//|| set->root * c->offset[0] + svec[0] < 0
			//|| set->root * c->offset[2] + svec[2] < 0
			) continue;
				

			uint32_t x = ws[0];
			uint32_t z = ws[2];

			float hz;

			//float h = get_height(set, x, z);
			uint32_t features = featuremap[ x | z * ws_max_x ];

			float h = features & 0xFFFF;

			ws[1] = h;

			if( features >> FEATMAP_TREE_BIT == 1){
				chunkset_gen_tree_in_chunk(set, c, ws);
			}

			if( svec[0] < 0 || svec[0]+1 > set->root
			||  svec[2] < 0 || svec[2]+1 > set->root ) 
				continue;

			uint16_t vec[3] = {0};
			for (int i = 0; i < 3; ++i)
				vec[i] = svec[i];


			for( vec[1] = 0; vec[1] < set->root; vec[1]++  )
			{
				ws[1] = set->root * c->offset[1] + vec[1] ;
				uint32_t i = flatten1( vec, set->root_bitw );

				int32_t depth = ws[1]-h;

				if( depth < 0 ){
					shadow_place_update(set, ws);
					c->voxels[i] = 21;
					if( depth == -1 ){
						//hz = 0.05;
						if( h < 8+1 )
							c->voxels[i] = 23;// 010111
						else if( 120+noise_simplex( x*1.0f, 0, z*1.0f )*20 > h )
							c->voxels[i] = 8;
						else{
							hz = 2.0f;
							if( 190+noise_u_simplex_f3( x*hz, 0, z*hz )*200 > h )
								c->voxels[i] = 42; // 101010
							else
								c->voxels[i] = 63;

						}
					}
				}

				//c->voxels[i] = depth;
			}


		}

		done++;

		chunk_close_rw(set, c);
		chunk_compress(set, c);
		
	}

	//chunkset_gen_trees( set );

	mem_free(featuremap);
}