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

struct ChunkSet * 
chunkset_create( uint8_t root_bitw, uint8_t max_bitw[] )
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
	
	return set;
}

void chunkset_clear( struct ChunkSet *set )
{
	logf_info( "Clearing" );
	uint32_t num_voxels = set->root * set->root * set->root;
		
	uint16_t vec[3];
	for( vec[0] = 0; vec[0] < set->max[0]; vec[0]++  )
	for( vec[1] = 0; vec[1] < set->max[1]; vec[1]++  )
	for( vec[2] = 0; vec[2] < set->max[2]; vec[2]++  )
	{

		uint32_t i = flatten3( vec, set->max_bitw );
//		logf_info( "Clearing chunk %iu", i );

		struct ChunkMD *c = &set->chunks[i];

		pthread_mutex_init(&c->mutex, NULL);

		c->lod = -1;
		c->last_access = ctx_time();
		c->count = num_voxels;
		c->voxels = mem_calloc( num_voxels * sizeof(Voxel) );
		memcpy( c->offset, vec, 3*sizeof(uint16_t) );

		c->dirty = 1;
		
		c->gl_vbo =  0;

	}

	// Nullchunk
	set->null_chunk = mem_calloc(sizeof(struct ChunkMD));
	struct ChunkMD *c = set->null_chunk;
	c->last_access = ctx_time();
	c->count = num_voxels;
	c->voxels = mem_calloc( num_voxels * sizeof(Voxel) );

}







void chunk_touch_ro( struct ChunkMD *c  )
{
	chunk_open_ro(c);
}


void chunk_open_ro( struct ChunkMD *c )
{
//	c->readers++;
	
	c->last_access = (uint32_t)ctx_time();
	if( c->voxels != NULL ) 
		return;

	pthread_mutex_lock( &c->mutex ); // Lock the chunk for decompression

	if( c->voxels != NULL ) { // DId another thread decompress it already?
		pthread_mutex_unlock( &c->mutex );
		return;
	} else if ( c->rle == NULL  ){
		// Unallocated, throw an error and exit!
		logf_error("Unimplemented: Can not read unallocated chunk");
		panic();
	}else{
		// Uncompress chunk!
		c->voxels = rle_decompress(c->rle);
		mem_free(c->rle);
		c->rle = NULL;
		pthread_mutex_unlock( &c->mutex );
		return;
	}
}

void chunk_close_ro( struct ChunkMD *c )
{
//	c->readers--;
//	c->last_access = (uint32_t)ctx_time();
}



void chunk_open_rw( struct ChunkMD *c )
{
	chunk_open_ro(c);
	// Unimplemented
}

void chunk_close_rw( struct ChunkMD *c )
{
	chunk_close_ro(c);
	// Unimplemented
}




// Checks chunk safety
// Is the voxel inside of a chunk, or its edge? 
int voxel_chunk_safe(
	struct ChunkSet *set,
	struct ChunkMD *c,
	uint16_t *vec,
	uint32_t vi
){
	uint16_t r = set->root-1;
	return !(
		vec[0] == 0 ||
		vec[1] == 0 ||
		vec[2] == 0 ||
		vec[0] == r ||
		vec[1] == r ||
		vec[2] == r 
	);

}

// Checks set safety
// Is the chunk on set edge? Is there neigboring chunks?
int voxel_set_safe(
	struct ChunkSet *set,
	struct ChunkMD *c,
	uint16_t *vec,
	uint32_t vi
){
	//chunk_touch_ro(c);
	// Calulate worldspace
	for( int i = 0; i < 3; i++  ){

		uint32_t max = (set->max[i]  << set->root_bitw) - 1;
		uint32_t  ws = (c->offset[i] << set->root_bitw) + vec[i] ;
		if( ws == 0 || ws == max) return 0;
	}
	
	return 1;

}

// Checks voxel visibility on chunk boundaries
// This function is SET UNSAFE!
int voxel_visible_safe(
	struct ChunkSet *set,
	struct ChunkMD *c,
	uint16_t *vec,
	uint32_t vi
){
	uint8_t shift = 0;
	uint8_t cshift = 0;

	uint32_t ci = flatten3( c->offset, set->max_bitw );

	uint16_t r = set->root-1;  
	// bitmask 0b 000 000 111
	//             X   Y   Z

	chunk_touch_ro(c);

	// Loop per face
	for( int i = 0; i < 3; i++  ){
		// -check
		if( vec[i] == 0  ){ 
			struct ChunkMD *nc = &set->chunks[ ci - (1 << cshift) ];
			chunk_touch_ro(nc);
			if(  nc->voxels[ vi + (r << shift) ] == 0 ) return 1;
		}else if( c->voxels[ vi - (1 << shift) ] == 0 ) return 1;
		// +check
		if( vec[i] == r ){ 
			struct ChunkMD *nc = &set->chunks[ ci + (1 << cshift) ];
			chunk_touch_ro(nc);
			if(  nc->voxels[ vi - (r << shift) ] == 0 ) return 1;
		}else if( c->voxels[ vi + (1 << shift) ] == 0 ) return 1;
		
		shift  += set->root_bitw;
		cshift += set-> max_bitw[i];
	}

	return 0;
}

// Chunk&set Unsafe visibility check
// Very fast, but segfaults if you check on chunk edges
int voxel_visible_unsafe( 
	struct ChunkSet *set,
	struct ChunkMD *c,
	uint16_t *vec,
	uint32_t vi
){	
	uint8_t shift = 0;
	for( int i = 0; i < 3; i++ ){
		if( c->voxels[ vi + (1 << shift) ] == 0 
		||  c->voxels[ vi - (1 << shift) ] == 0  
		) return 1;
		shift += set->root_bitw;
	}
	return 0;
}


// Set and chunk safe visibility check
int voxel_visible(
	struct ChunkSet *set,
	struct ChunkMD *c,
	uint16_t *vec,
	uint32_t vi
){

	if( !voxel_set_safe( set,c,vec,vi ) ) {
		return !(vec[1] == 0);
	}

	if( voxel_chunk_safe( set,c,vec,vi ) )
		return voxel_visible_unsafe( set,c,vec,vi );
	else
		return voxel_visible_safe  ( set,c,vec,vi );
}


void chunk_compress(struct ChunkMD *c){

	void *vxl = c->voxels;
	c->voxels = NULL;
	pthread_mutex_lock( &c->mutex );

	c->rle = rle_compress( vxl, c->count  );
	mem_free( vxl );
	pthread_mutex_unlock( &c->mutex );
}



//1int16_t*	mesher_geom[128] = {NULL};
//1void* 		mesher_work[128] = {NULL};
//1uint8_t		mesher_lock[128] = {0};


#define MESHER_THREAD_BUFFERS 3

struct {
	void		*geom[MESHER_THREAD_BUFFERS];
	uint32_t	 geom_size;
	void		*work[MESHER_THREAD_BUFFERS];
	uint32_t	 work_size;
	uint8_t		 lock[MESHER_THREAD_BUFFERS];
} mesher[16];

void chunkset_manage(
	struct ChunkSet *set
){

	// Create a temporary work buffer
	//float * vertex = mem_calloc( sizeof(float) * *vertex_count );
	int meshed_count = 0;

	#pragma omp parallel for
	for( int i = 0; i < set->count; i++ ){
		if(meshed_count>64) continue;

		int omp_id = omp_get_thread_num();

		int buf_id;
		for (buf_id = 0; buf_id < MESHER_THREAD_BUFFERS; ++buf_id) {
			
			if ( mesher[omp_id].lock[buf_id] ) continue;

			if( mesher[omp_id].geom[buf_id] == NULL ) {
				// Allocate!
				mesher[omp_id].geom_size = set->root*set->root*set->root*10;

				mesher[omp_id].geom[buf_id] = 
					mem_calloc( mesher[omp_id].geom_size );

				mesher[omp_id].work_size = set->root*2*set->root*2*set->root*2*sizeof(uint8_t);

				mesher[omp_id].work[buf_id] = 
					mem_calloc( mesher[omp_id].work_size );
			}
			// We found a free allocated buffer, break!
			break; 
		}
		if( buf_id == MESHER_THREAD_BUFFERS ) {
			//logf_warn( "Mesher thread ran out of buffers" );
			continue;
		}

		struct ChunkMD *c = &set->chunks[i];

		if( c->dirty == 0 && 
			c->gl_vbo_local_lod == c->lod 
		) {
			if( c->rle == NULL 
			&&	c->last_access+1 < ctx_time() ){
				chunk_compress(c);
				meshed_count++;
			}
			continue;
		}

		if( c->lod == -1 ) continue;
		if( c->gl_vbo_local ) continue;

		chunk_open_ro(c);

		// Remember, meshing takes time!!! things may change!!!
		// Mark now, if theres a write while we do stuff, let it happen
		c->dirty = 0;
		c->gl_vbo_local_lod = c->lod;

		memset( mesher[omp_id].work[buf_id], 0, 
				mesher[omp_id].work_size );

		double t = ctx_time();
		uint32_t geom_items = 0; 
		uint32_t indx_items = 0; 
		if( c->lod == 0 )
			 chunk_make_mesh(
				set, c, 
				mesher[omp_id].geom[buf_id], &geom_items, 
				mesher[omp_id].work[buf_id], &indx_items
			);
		else
			chunk_make_splatlist(
				set, c, 
				mesher[omp_id].geom[buf_id], &geom_items, 
				mesher[omp_id].work[buf_id]
			);

		t = ctx_time()-t;
		//logf_info("Meshing time: %i ms (%i)", (int)(t*1000), geom_items);


		c->gl_vbo_local_items = geom_items;
		c->gl_ibo_local_items = indx_items;

		if( indx_items > 0 ){
			//uint32_t *buf = mem_alloc( 	  indx_items*sizeof(uint32_t) );
			//memcpy( buf, mesher_work[id], indx_items*sizeof(uint32_t) );
			//c->gl_ibo_local = buf;
			c->gl_ibo_local = mesher[omp_id].work[buf_id];
		}
		if( geom_items > 0  ){
			//float *buf = mem_alloc( 	  geom_items*sizeof(float) );
			//memcpy( buf, mesher_geom[id], geom_items*sizeof(float) );
			mesher[omp_id].lock[buf_id]++;
			c->mesher_lock = &mesher[omp_id].lock[buf_id];
			//c->gl_vbo_local = buf;
			c->gl_vbo_local = mesher[omp_id].geom[buf_id];
		}

		
		// POTENTIAL BUG: IF CHUNK CLEARED, MAKE SURE YOU CLEAR THE GEOMETRY!
		// Making this into a zero can be used to signal about this state!

		chunk_close_ro(c);
		meshed_count++;
	}
}





uint32_t flatten3( const uint16_t* l, const uint8_t* b )
{
	uint32_t i = l[2];

	i<<= b[1];
	i |= l[1];

	i<<= b[0];
	i |= l[0];

	return i;
}

uint32_t flatten1( const uint16_t* l, const uint16_t b )
{
	uint32_t i = l[2];

	i<<= b;
	i |= l[1];

	i<<= b;
	i |= l[0];

	return i;
}
