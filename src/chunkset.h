#ifndef CHUNKSET_H
#define CHUNKSET_H 1


#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>


typedef uint8_t Voxel;

struct ChunkSet {
	
	uint32_t count;			// Total chunk count

	uint16_t  root; 		// Cuberoot of chunks
	uint8_t   root_bitw;	// Bitwidth of root

	uint16_t  max[3];		// Amount of chunks per dimension
	uint8_t   max_bitw[3];	// Bitwidth of max
	
	// Size of world can be counted by max[i]*root

	struct ChunkMD *chunks;

	struct ChunkMD *null_chunk;

	uint16_t *shadow_map;
	uint32_t shadow_map_length;
	uint32_t shadow_map_size[2];
	
};

#define MAX_LOD_LEVEL 8

// Chunk MetaData
// this is getting bloated, figure something out 
struct ChunkMD {
	
	pthread_mutex_t mutex_read;
	pthread_mutex_t mutex_write;
	uint16_t readers;		// num threads are reading the chunk (UNUSED)

	float last_access;	// ctx_time()
	uint32_t count;			// Count of voxels (duplicated just for convinience...)
	uint16_t offset[3];		// In chunks
	Voxel  *voxels;			
	Voxel  *rle;
	
	int8_t		lod;
	uint8_t     dirty;				// Bool for geometry outdated

	void	   *gl_vbo_local;		// Temporary store for vbo data
	uint8_t		gl_vbo_local_lod;
	uint32_t	gl_vbo_local_items;
	uint32_t	gl_vbo_local_segments[MAX_LOD_LEVEL];

	void	   *gl_ibo_local;
	uint32_t	gl_ibo_local_items;

	uint32_t	gl_vao;
	uint32_t	gl_vbo;
	uint8_t		gl_vbo_lod;
	uint32_t	gl_vbo_segments[MAX_LOD_LEVEL];
	uint32_t	gl_vbo_items;

	uint32_t	gl_ibo;
	uint32_t	gl_ibo_items;

	uint8_t	    *mesher_lock;

	// use this in the future?
/*
	// REMOTE
	uint8_t		splat_dirty;
	uint8_t		 mesh_dirty;

	uint32_t	gl_splat_vbo;
	uint32_t	gl_splat_vbo_items[MAX_LOD_LEVEL];

	uint32_t	gl_mesh_vbo;
	uint32_t	gl_mesh_vbo_items;
	uint32_t	gl_mesh_ibo;
	uint32_t	gl_mesh_ibo_items;

	// LOCAL
	uint32_t	lo_splat_vbo;
	uint32_t	lo_splat_vbo_items[MAX_LOD_LEVEL];

	uint32_t	lo_mesh_vbo;
	uint32_t	lo_mesh_vbo_items;
	uint32_t	lo_mesh_ibo;
	uint32_t	lo_mesh_ibo_items;
*/
};

// Some sort of documentation

/*
 * ChunkMD states
 *
 * Voxel Data
 * State is determined by pointers:
 *   voxels and rle are NULL
 *   	Initialize the set!
 *
 *   voxels is defined, rle NULL;
 *		Chunk has been written to, RLE has to be regenrated
 *		
 *   voxels is defined, rle is defined
 *  	Chunk was in read only mode. RLE has to be deleted once any write occurs
 *   
 *   voxels NULL, rle defined
 *  	Requires decompressing on open
 *
 * 	The nullchunk
 *   A read-only chunk with nothing but air. Copy on modify
 *
 *    
 *
 * Geometry
 *	setting dirty invalidates VBO
 *	vbo_local is used to store and commit changes to geometry
 *  - Set all the other variables like item count before setting vbo_local !!!
 *
 * TODO
 * 	Make sure everything works properly when chunk doenst anymore generate a 
 *  mesh (when all the voxels are removed)
 *   - Hacky fixed, clean up properly still to be done
 */


/*
 * Types 
 *
 * Cuberoots: uint8_t
 * Bitwidth values: uint8_t
 *
 * Iterators: uint32_t
 *
 * Vector: uint16_t [3]
 * 		location of a chunk or a voxel inside a chunk
 *
 * Worldspace vector: uint32_t
 *
 */

/*
 * Bitshift magic
 * 
 * With 2^N cuberoots, the iterators happen to contain the 3d coordinates:
 *		X	Y	Z
 *	0b 110 010 001, INDEX = 86 of a 8*8*8 volume.
 *	    6   2   1
 * 
 * INDEX += n << 3 moves index in Y+n
 * bitw means "bit width"
 *
 * root_bitw = log2(root);
 * root = 1 << root_bitw;
 * For chunk coordinates the bitwidths may be different per dimension
 */


struct ChunkSet* 
chunkset_create( uint8_t, uint8_t[]);
void chunkset_clear( struct ChunkSet* );
void chunkset_clear_import( struct ChunkSet* );

void chunkset_manage( struct ChunkSet* );

void chunk_open_ro(  struct ChunkSet *set, struct ChunkMD *c );
void chunk_open_rw(  struct ChunkSet *set, struct ChunkMD *c );
void chunk_close_ro( struct ChunkSet *set, struct ChunkMD *c );
void chunk_close_rw( struct ChunkSet *set, struct ChunkMD *c );

Voxel voxel_read( struct ChunkSet*, uint32_t*  );
void  voxel_write( struct ChunkSet*, uint32_t*, Voxel );

void chunk_compress(struct ChunkSet *set, struct ChunkMD *c);

void chunkset_force_compress( struct ChunkSet* );

// Potentially globally useful helpers
uint32_t flatten3( const uint16_t* l, const uint8_t* b );
uint32_t flatten1( const uint16_t* l, const uint16_t b );

uint8_t sample_shadow( 
	struct ChunkSet *set, 
	uint32_t *ws 
);
uint8_t sample_shadow_c( 
	struct ChunkSet *set, 
	struct ChunkMD *c, 
	uint16_t *ws 
);
void chunkset_create_shadow_map( 
	struct ChunkSet *set
);
void shadow_update(
	struct ChunkSet *set,
	uint32_t *ws
);
void shadow_place_update(
	struct ChunkSet *set,
	uint32_t *ws
);
void shadow_break_update(
	struct ChunkSet *set,
	uint32_t *ws
);
uint32_t shadow_map_index( 
	struct ChunkSet *set, 
	uint32_t x, 
	uint32_t y 
);



#endif
