#ifndef CHUNKSET_H
#define CHUNKSET_H 1


#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>


typedef uint8_t Voxel;


/*
 * TODO
 * 	Make sure everything works properly when chunk doesn't anymore generate a 
 *  mesh (when all the voxels are removed)
 *   - Nothng is rendered, but vaos etc are not cleaned up properly
 *
 *  Separate most OpenGL stuff from the chunks, and put them in an octree
 *  LOD should work like this https://kosshi.net/u/jzctt.html
 */


// A block of chunked voxel space
struct ChunkSet {
	
	uint32_t count;			// Total chunk count, DEPRICATED

	uint32_t chunk_count;	// Total chunk count (UNUSED ATM)
	uint32_t voxel_count;	// Voxel count in a single chunk (!!!) (UNUSED ATM)

	uint16_t  root; 		// Cuberoot of chunks
	uint8_t   root_bitw;	// Bitwidth of root

	uint16_t  max[3];		// Amount of chunks per dimension
	uint8_t   max_bitw[3];	// Bitwidth of max
	
	// Size of world can be counted by max[i]*root

	struct ChunkMD *chunks;
	struct ChunkMD *null_chunk; // Read only chunk with nothing but air

	uint16_t *shadow_map;
	uint32_t  shadow_map_length;
	uint32_t  shadow_map_size[2];
};


#define MAX_LOD_LEVEL 8

// Chunk MetaData
// this is getting bloated, figure something out 
struct ChunkMD {
	
	// R/W Lock implementation
	pthread_mutex_t mutex_read;
	pthread_mutex_t mutex_write;
	uint16_t readers;

	float last_access;		// ctx_time()
	float last_meshing;		// Used to limit meshing frequency

	uint32_t count;			// Count of voxels, DEPRICATED
	uint16_t offset[3];		// In chunks
	Voxel 	*voxels;			
	Voxel 	*rle;

	int8_t		lod;		// Level of detail. -1 is meshed
	uint8_t     dirty;		// Signal geometry update

	void	   *gl_vbo_local;  	
	// Temporary store for vbo data. Also a signal for upload. Do not free!
	int8_t		gl_vbo_local_lod;
	uint32_t	gl_vbo_local_items;
	uint32_t	gl_vbo_local_segments[MAX_LOD_LEVEL];

	void	   *gl_ibo_local; 
	uint32_t	gl_ibo_local_items;

	uint32_t	gl_vao;
	uint32_t	gl_vbo;
	int8_t		gl_vbo_lod;
	uint32_t	gl_vbo_segments[MAX_LOD_LEVEL];
	uint32_t	gl_vbo_items;

	uint32_t	gl_ibo;
	uint32_t	gl_ibo_items;

	// Signal to mesher that buffers are safe for reuse
	uint8_t	    *mesher_lock; 

	// use this in the future?
/*
	// Sparse voxel list, basically just list of visible voxels
	void 	*svl [MAX_LOD_LEVEL];
	uint32_t svl_items;

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

/*
 * Synchronization
 * For voxel access, you must use open/close functions below.
 * For communication between meshing and rendering threads, no locks should be 
 * used. Instead, changing certain variables will be used for signaling.
 */

/*
 * Functions below lock/unlock chunk voxel data and handle decompression and
 * other behind the scenes magic.
 * You can obtain multiple read locks, but never ever obtain multiple write 
 * locks! Or a write lock and a read lock!
 */

void chunk_open_ro(  struct ChunkSet *set, struct ChunkMD *c );
void chunk_open_rw(  struct ChunkSet *set, struct ChunkMD *c );
void chunk_close_ro( struct ChunkSet *set, struct ChunkMD *c );
void chunk_close_rw( struct ChunkSet *set, struct ChunkMD *c );

/*
 * Exclusive locks with no decompression or magic. Should not be used.
 */

void chunk_lock(   struct ChunkSet *set, struct ChunkMD *c );
void chunk_unlock( struct ChunkSet *set, struct ChunkMD *c );


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
 *   Everything above has been implemented in the open/close functions
 * 
 *
 */

/*
 * Bitshift magic
 * 
 * With 2^N cuberoots, the iterators "happen" to contain the 3d coordinates:
 *      X   Y   Z
 *  0b 110 010 001, INDEX = 86 of a 8*8*8 volume.
 *      6   2   1
 * 
 * INDEX += n << 3 moves index in Y+n
 *
 * bitw means "bit width"
 * root_bitw = log2(root);
 * log2 for po2 numbers can be implemented with __built_in_ctz 
 *
 * root = 1 << root_bitw;
 *
 * For chunk coordinates the bitwidths may be different per dimension
 */


/*
 * Other notes
 * 
 * Any function that takes a chunk, also should take the set
 *
 */


/*
 * Types 
 *
 * Cuberoots: uint8_t
 * Bitwidth values: uint8_t
 *
 * Iterators: uint32_t
 *
 * Vector: uint16_t
 * 		location of a chunk or a voxel inside a chunk
 *
 * Worldspace vector: uint32_t
 *  - Actually, use int32_t ?
 *
 */


// Flaten a coordinate. 
uint32_t flatten1( const uint16_t* l, const uint8_t bitw );
// In case of non-cube volume:
uint32_t flatten3( const uint16_t* l, const uint8_t* bitw );


struct ChunkSet* chunkset_create( uint8_t, uint8_t[]);
void chunkset_clear( struct ChunkSet* );
void chunkset_clear_import( struct ChunkSet* );

void chunkset_manage( struct ChunkSet* );

void chunk_compress(struct ChunkSet *set, struct ChunkMD *c);


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
