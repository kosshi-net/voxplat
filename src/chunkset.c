#define __FILENAME__ "chunkset.c"
#include "chunkset.h"

#include "event.h"
#include "ctx.h"
#include "mem.h"

#include "cpp/noise.h"

#include "chunkset/edit.h"
#include "chunkset/mesher.h"

#include <stdlib.h>
#include <stdint.h>

#include <pthread.h>
#include <string.h>

#include <math.h>
#include <cglm/cglm.h>

#include <chunkset/rle.h>

#include <omp.h>


//#define DISABLE_SHADOWS
#include "chunkset/shadow.h"


struct ChunkSet * 
chunkset_create( uint8_t root_bitw, uint8_t max_bitw[3] )
{	
	uint16_t max[3] = {
		1 << max_bitw[0],
		1 << max_bitw[1],
		1 << max_bitw[2]
	};
	
	uint32_t chunk_count = max[0]*max[1]*max[2];

	logf_info( "Creating chunkset" );
	
	struct ChunkSet *set = mem_calloc( 
		sizeof(struct ChunkSet) + chunk_count * sizeof(struct ChunkMD)
	);
		
//	struct ChunkSet *set = mem_calloc( sizeof(struct ChunkSet) );
		
	set->count = chunk_count;

	set->root_bitw = root_bitw; // 3
	set->root = 1 << root_bitw; // 8

	memcpy( set->max_bitw, max_bitw, 3);
	memcpy( set->max, max, 3*sizeof(uint16_t) );


	logf_info( "max %i %i %i", set->max[0], set->max[1], set->max[2] );
	logf_info( "max_bitw %i %i %i r %i", 
		set->max_bitw[0], 
		set->max_bitw[1],
		set->max_bitw[2],
		set->root
	);
	logf_info( "World is %i x %i x %i with %i^3 chunks", 
		set->max[0] * set->root,
		set->max[1] * set->root,
		set->max[2] * set->root,
		set->root
	);
	
	// Point to memory right after the struct, the metadata array is there
	set->chunks = (void*)set + sizeof(struct ChunkSet);
//	set->chunks = mem_calloc( sizeof( struct ChunkMD ) * chunk_count  );
	
	for (int i = 0; i < MAX_LOD_LEVEL; ++i)
		set->gsvl[i] = mem_calloc( sizeof( struct GeometrySVL ) * chunk_count );

	set->gmesh = mem_calloc( sizeof( struct GeometryMesh ) * chunk_count );
	
	return set;
}

void chunkset_clear( struct ChunkSet *set )
{
	logf_info( "Clearing" );
	uint32_t num_voxels = set->root * set->root * set->root;
	uint32_t i = 0;
	uint16_t vec[3];

	// Nullchunk
	set->null_chunk = mem_calloc(sizeof(struct ChunkMD));
	struct ChunkMD *c = set->null_chunk;
	c->last_access = ctx_time();
	c->count = num_voxels;
	c->voxels = mem_calloc( num_voxels * sizeof(Voxel) );
	c->rle = rle_compress( c->voxels, c->count);

	// Normal chunks
	for( vec[2] = 0; vec[2] < set->max[2]; vec[2]++  )
	for( vec[1] = 0; vec[1] < set->max[1]; vec[1]++  )
	for( vec[0] = 0; vec[0] < set->max[0]; vec[0]++  )
	{
		struct ChunkMD *c = &set->chunks[i];

		pthread_mutex_init(&c->mutex_read, NULL);
		pthread_mutex_init(&c->mutex_write, NULL);
		pthread_mutex_init(&c->mutex_svl, NULL);

		c->last_access = ctx_time();
		c->count = num_voxels;

		memcpy( c->offset, vec, 3*sizeof(uint16_t) );

		c->dirty = 1;
		c->rle = set->null_chunk->rle;

		i++;
	}
}


uint32_t flatten3( const uint16_t l[3], const uint8_t b[3] ) {
	return ( l[2] << b[1] | l[1]) << b[0] | l[0];
}

uint32_t flatten1( const uint16_t l[3], const uint8_t b ) {
	return ( l[2] << b | l[1]) << b | l[0];
}


void chunk_open_ro( struct ChunkSet *set, struct ChunkMD *c )
{
	pthread_mutex_lock( &c->mutex_read ); 
	c->readers++;

	// rw lock
	if( c->readers == 1 ) {
		pthread_mutex_lock( &c->mutex_write );
	}
	
	if(!c->voxels) {
		if( c->rle == set->null_chunk->rle )
			c->voxels = set->null_chunk->voxels;
		 else
			c->voxels = rle_decompress( c->rle );	
	}

	c->last_access = (uint32_t)ctx_time();
	pthread_mutex_unlock( &c->mutex_read );
}

void chunk_close_ro( struct ChunkSet *set, struct ChunkMD *c )
{
	pthread_mutex_lock( &c->mutex_read ); 
	c->readers--;
	if(	c->readers == 0 ) {
		pthread_mutex_unlock( &c->mutex_write ); 
	}
	pthread_mutex_unlock( &c->mutex_read ); 

	c->last_access = (uint32_t)ctx_time();
}


void chunk_open_rw( struct ChunkSet *set, struct ChunkMD *c )
{
	pthread_mutex_lock( &c->mutex_write ); 

	c->last_access = (uint32_t)ctx_time();

	// Null chunk
	if( c->voxels == set->null_chunk->voxels
	||  c->rle    == set->null_chunk->rle ) {
		c->rle = NULL;
		c->voxels = mem_alloc( c->count*sizeof(Voxel) );
		memcpy(c->voxels, set->null_chunk->voxels, c->count*sizeof(Voxel) );
		return;
	}

	if( c->rle && c->voxels ) {
		c->rle = mem_free(c->rle);
		return;
	}

	if( c->rle && !c->voxels ){
		c->voxels = rle_decompress( c->rle );
		c->rle = mem_free( c->rle );
		return;
	}

	if(c->voxels) return;

	logf_error("Uninitalized chunk?");
	logf_info("vxl %p rle %p", c->voxels, c->rle);
	logf_info("nullvxl %p rle %p", set->null_chunk->voxels, set->null_chunk->rle);
	panic();
}

void chunk_close_rw( struct ChunkSet *set, struct ChunkMD *c ){
	c->last_access = (uint32_t)ctx_time();
	pthread_mutex_unlock( &c->mutex_write );
}

void chunk_lock(   struct ChunkSet *set, struct ChunkMD *c ){
	pthread_mutex_lock( &c->mutex_write );
}
void chunk_unlock( struct ChunkSet *set, struct ChunkMD *c ){
	pthread_mutex_unlock( &c->mutex_write );
}

void chunk_compress(struct ChunkSet *set, struct ChunkMD *c){
	if(!c->voxels) return;

	if( c->voxels == set->null_chunk->voxels ) {
		c->voxels = NULL;
	} else if( c->rle ) {
		c->voxels = mem_free(c->voxels);
	} else {

		c->rle = rle_compress(c->voxels, c->count);
		c->voxels = mem_free(c->voxels);
	
		if( *((uint32_t*)c->rle) == *((uint32_t*)set->null_chunk->rle) ){
			c->rle = mem_free(c->rle);
			c->rle = set->null_chunk->rle;
		}
	}
}


#define MESHER_THREAD_BUFFERS 2

struct {
	void		*geom[MESHER_THREAD_BUFFERS];
	void		*mask[MESHER_THREAD_BUFFERS];
	void		*work[MESHER_THREAD_BUFFERS];
	uint32_t	 work_size;
	uint32_t	 geom_size;
	uint32_t	 mask_size;

	uint8_t		 lock[MESHER_THREAD_BUFFERS];
} mesher[4];

void chunkset_manage(
	struct ChunkSet *set
){
	int meshed_count = 0;

	#pragma omp parallel num_threads(4)
	#pragma omp for 
	for( int i = 0; i < set->count; i++ ){

		if(meshed_count>8) continue;

		int omp_id = omp_get_thread_num();
		int buf_id;
		// Find a free buffer
		for (buf_id = 0; buf_id < MESHER_THREAD_BUFFERS; ++buf_id) {
			if( mesher[omp_id].lock[buf_id] ) continue;
			if( mesher[omp_id].geom[buf_id] == NULL ) { // alloc
				mesher[omp_id].geom_size = pow(set->root, 3) * 10;
				mesher[omp_id].work_size = pow(set->root+1, 3) * 10;
				mesher[omp_id].mask_size = pow(set->root+1, 3);
				mesher[omp_id].geom[buf_id] = mem_calloc( mesher[omp_id].geom_size );
				mesher[omp_id].work[buf_id] = mem_calloc( mesher[omp_id].work_size );
				mesher[omp_id].mask[buf_id] = mem_calloc( mesher[omp_id].mask_size );
			}
			break;  // Found free allocated buffer
		}
		if( buf_id == MESHER_THREAD_BUFFERS ) {
			//logf_warn( "Mesher thread ran out of buffers" );
			continue;
		}


		// Ignore everything above


		struct ChunkMD *c = &set->chunks[i];

		// Dont mesh if previous geometry still not uploaded
		if( c->svl_dirty || c->mesh_dirty ) continue;


		c->remesh = c->remesh | c->dirty;
		if( c->remesh == 0 ){

			if( !c->mesh_dirty
			&&	 c->mesh_vbo 
			){
				c->mesh_vbo = mem_free(c->mesh_vbo);
				c->mesh_ibo = mem_free(c->mesh_ibo);
			}


			// COMPRESS
			if( c->voxels
			&&	c->last_access+1.0 < ctx_time() 
			){ 
				chunk_lock(set, c);
				chunk_compress(set, c);
				chunk_unlock(set, c);
			}
			continue;
		}
		// Dont mesh or access too fast
		if(c->last_meshing+0.100 > ctx_time() ) continue;

		c->changing = c->dirty;
		c->dirty = 0;
		c->remesh = 0;

		// Mesh, something
		c->last_meshing = ctx_time();

		chunk_open_ro(set, c);


		memset( mesher[omp_id].work[buf_id], 0, 
				mesher[omp_id].work_size );

		memset( mesher[omp_id].mask[buf_id], 0, 
				mesher[omp_id].mask_size );

		memset( mesher[omp_id].geom[buf_id], 0, 
				mesher[omp_id].geom_size );

		uint32_t geom_items = 0; 
		uint32_t indx_items = 0; 

		uint32_t temp_lo_vbo_segments[ MAX_LOD_LEVEL ] = {0};

		uint8_t clear_svl = 0;

		if( c->make_mesh ) {
			double t = ctx_time();
			 chunk_make_mesh(
				set, c, 
				mesher[omp_id].geom[buf_id], &geom_items, 
				mesher[omp_id].work[buf_id], &indx_items
			);
			t = ctx_time()-t;
			//logf_info("Meshing time: %f ms (%i)", (t*1000.0), geom_items);

			if( c->mesh_vbo ) mem_free( c->mesh_vbo );
			c->mesh_vbo_items = geom_items;
			c->mesh_vbo = mem_alloc( c->mesh_vbo_items * sizeof(uint16_t) );
			memcpy(   
				c->mesh_vbo,
				mesher[omp_id].geom[buf_id],
				c->mesh_vbo_items * sizeof(uint16_t)
			);

			if( c->mesh_ibo ) mem_free( c->mesh_ibo );
			c->mesh_ibo_items = indx_items;
			c->mesh_ibo = mem_alloc( c->mesh_ibo_items * sizeof(uint32_t) );
			memcpy(   
				c->mesh_ibo,
				mesher[omp_id].work[buf_id],
				c->mesh_ibo_items * sizeof(uint32_t)
			);

			clear_svl = c->no_geometry = !geom_items;
			c->mesh_dirty = 1;

		} else {

			double t = ctx_time();
			chunk_make_mask( set, c, mesher[omp_id].mask[buf_id] );

			t = ctx_time()-t;
			//logf_info("SVLMask time: %f ms (%i)", (t*1000.0), geom_items);

			uint32_t geom_items2 = 0;

			// TEMP CODE TEMP CODE 
			{
				chunk_make_splatlist(
					set, c, 0,
					mesher[omp_id].mask[buf_id],
					mesher[omp_id].geom[buf_id], 
					&geom_items
				);
				temp_lo_vbo_segments[0] = geom_items;



				chunk_mask_downsample( set, 1, 
					mesher[omp_id].mask[buf_id],
					mesher[omp_id].work[buf_id]
				);

				geom_items2 = 0;
				chunk_make_splatlist(
					set, c, 1,
					mesher[omp_id].work[buf_id],
					&mesher[omp_id].geom[buf_id][geom_items*sizeof(uint16_t)], 
					&geom_items2
				);
				temp_lo_vbo_segments[1] = geom_items2;
				geom_items+=geom_items2;




				memset( mesher[omp_id].mask[buf_id], 0, mesher[omp_id].mask_size );
				chunk_mask_downsample( set, 2, 
					mesher[omp_id].work[buf_id],
					mesher[omp_id].mask[buf_id]
				);
				
				geom_items2 = 0;
				chunk_make_splatlist(
					set, c, 2,
					mesher[omp_id].mask[buf_id],
					&mesher[omp_id].geom[buf_id][geom_items*sizeof(uint16_t)], 
					&geom_items2
				);
				temp_lo_vbo_segments[2] = geom_items2;
				geom_items+=geom_items2;
				



				memset( mesher[omp_id].work[buf_id], 0, mesher[omp_id].work_size );
				chunk_mask_downsample( set, 3, 
					mesher[omp_id].mask[buf_id],
					mesher[omp_id].work[buf_id]
				);
				
				geom_items2 = 0;
				chunk_make_splatlist(
					set, c, 3,
					mesher[omp_id].work[buf_id],
					&mesher[omp_id].geom[buf_id][geom_items*sizeof(uint16_t)], 
					&geom_items2
				);
				temp_lo_vbo_segments[3] = geom_items2;
				geom_items+=geom_items2;

				
				memset( mesher[omp_id].mask[buf_id], 0, mesher[omp_id].mask_size );
				chunk_mask_downsample( set, 4, 
					mesher[omp_id].work[buf_id],
					mesher[omp_id].mask[buf_id]
				);
				
				geom_items2 = 0;
				chunk_make_splatlist(
					set, c, 4,
					mesher[omp_id].mask[buf_id],
					&mesher[omp_id].geom[buf_id][geom_items*sizeof(uint16_t)], 
					&geom_items2
				);
				temp_lo_vbo_segments[4] = geom_items2;
				geom_items+=geom_items2;
				
			}
			// TEMP CODE TEMP CODE	

			pthread_mutex_lock( &c->mutex_svl );

			c->no_geometry = !geom_items;
			clear_svl = c->no_geometry;
			if( !clear_svl ) {

				for (int i = 0; i < MAX_LOD_LEVEL; ++i)
					c->svl_items[i] = temp_lo_vbo_segments[i];

				if( c->svl ) 
					c->svl = mem_free( c->svl );
			
				c->svl = mem_alloc( geom_items*sizeof(uint16_t) );
		
				memcpy(c->svl, 
					mesher[omp_id].geom[buf_id],
					geom_items*sizeof(uint16_t)
				);
				

				c->svl_items_total = geom_items;
				c->svl_dirty = 1;

			}
			pthread_mutex_unlock( &c->mutex_svl );

		}

		if( clear_svl ){
			pthread_mutex_lock( &c->mutex_svl );

			if( c->svl ) c->svl = mem_free( c->svl );
			memset( c->svl_items, 0, sizeof( temp_lo_vbo_segments ) );

			c->svl_items_total = 0;
			c->svl_dirty = 1;

			pthread_mutex_unlock( &c->mutex_svl );
		}

		c->changing = 0;
		chunk_close_ro(set, c);
		meshed_count++;
	}
}



