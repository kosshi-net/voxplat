#ifndef CHUNKSET_H
#define CHUNKSET_H 1


#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#include <stdatomic.h>
#define atomic _Atomic


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


// Chunked voxel space
struct ChunkSet {
	
	uint32_t count;			// Total chunk count, DEPRICATED

	uint32_t chunk_count;	// Total chunk count (UNUSED ATM)
	uint32_t voxel_count;	// Voxel count in a single chunk (!!!) (UNUSED ATM)

	uint16_t  root; 		// Cuberoot of chunks
	uint8_t   root_bitw;	// Bitwidth of root

	uint16_t  max[3];		// Amount of chunks per dimension
	uint8_t   max_bitw[3];	// Bitwidth of max
	
	// Size of world can be counted by max[i]<<root_bitw

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

	// Accessed with locks
	float last_access;		// ctx_time()
	float last_meshing;		// Used to limit meshing frequency

	uint32_t count;			// Count of voxels, DEPRICATED
	uint16_t offset[3];		// In chunks
	Voxel 	*voxels;			
	Voxel 	*rle;

	// Lockless accesses

	// Level of detail. -1 is used for meshing, 0 to MAX_LOD_LEVEL for 
	// splatting. Changing this from splat range to mesh range signals a remesh
	volatile int8_t		 lod;

	// Writing 1 signals a geometry update
	volatile uint8_t      dirty;		

	// Pointer to vbo data. Also a signal for upload. Do not free!
	void volatile   	*gl_vbo_local;  	
	int8_t			 	 gl_vbo_local_lod;
	uint32_t			 gl_vbo_local_items;
	uint32_t		 	 gl_vbo_local_segments[MAX_LOD_LEVEL];

	void				*gl_ibo_local; 
	uint32_t			 gl_ibo_local_items;
	// Signal to mesher that buffers are safe for reuse
	uint8_t volatile  	*mesher_lock; 

	// Owned excusively by rendering thread
	uint32_t	gl_vao;
	uint32_t	gl_vbo;
	int8_t		gl_vbo_lod;
	uint32_t	gl_vbo_segments[MAX_LOD_LEVEL];
	uint32_t	gl_vbo_items;

	uint32_t	gl_ibo;
	uint32_t	gl_ibo_items;
};

/*
 * Synchronization
 * For voxel access, you must use open/close functions below.
 *
 *
 * The volatile keyword is used here to mark variables that are
 * used by multiple threads with no mutexes. Synchronization for these is 
 * achieved by certain "signal" variables, writing to them means you give 
 * ownership to a different thread, and you must not change any relevant
 * variables (payload) anymore.
 * This is done to avoid using any locks in the rendering thread.
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
 * Everything above has been implemented in the open/close functions.
 * RO functions also return null chunks when possible, and RW properly 
 * copies.
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
