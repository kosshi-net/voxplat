#ifndef CHUNKSET_H
#define CHUNKSET_H 1


#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

typedef uint8_t Voxel;

#define MAX_LOD_LEVEL 5

struct GeometrySVL {
	uint8_t 	lod;
	uint32_t	vao;
	uint32_t	vbo;
	uint32_t	vbo_items;
};

struct GeometryMesh {
	uint32_t	vao;
	uint32_t	vbo;
	uint32_t	vbo_items;
	uint32_t	ibo;
	uint32_t	ibo_items;
};

// Chunked voxel space
struct ChunkSet {
	
	uint32_t count;			// Total chunk count, DEPRICATED

	uint32_t chunk_count;	// Total chunk count (UNUSED ATM)
	uint32_t voxel_count;	// Voxel count in a single chunk (!!!) (UNUSED ATM)

	uint16_t  root; 		// Cuberoot of chunks
	uint8_t   root_bitw;	// Bitwidth of root

	uint16_t  max[3];		// Amount of chunks per dimension
	uint8_t   max_bitw[3];	// Bitwidth of max
	

	struct GeometrySVL *gsvl [MAX_LOD_LEVEL];
	struct GeometryMesh*gmesh;

	// Size of world can be counted by max[i]<<root_bitw

	struct ChunkMD *chunks;
	struct ChunkMD *null_chunk; // Read only chunk with nothing but air

	uint16_t *shadow_map;
	uint32_t  shadow_map_length;
	uint32_t  shadow_map_size[2];
};



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


	pthread_mutex_t mutex_svl;

	// Lockless accesses

	// Level of detail. -1 is used for meshing, 0 to MAX_LOD_LEVEL for 
	// splatting. Changing this from splat range to mesh range signals a remesh

	// Set when voxels have changed
	volatile uint8_t 	 dirty;    

	// True when under meshing after dirty flag
	volatile uint8_t 	 changing; 

	// Set when you want a quiet remesh
	volatile uint8_t 	 remesh;   

	// True if should make a mesh
	volatile uint8_t	 make_mesh; 

	// svl available
	volatile uint8_t 	 svl_dirty;

	// mesh available
	volatile uint8_t 	 mesh_dirty; 

	// No geometry generated, clean up
	volatile uint8_t 	 no_geometry; 

	// Used and tracked by renderer for deleting old buffers
	int8_t 	 		 	 mesh_has_vbo; 

	// Sparse voxel list
	uint16_t			*svl; 
	uint32_t		 	 svl_items[MAX_LOD_LEVEL];
	uint32_t 			 svl_items_total;

	// Mesh geometry
	void 			  	*mesh_vbo;  
	uint32_t			 mesh_vbo_items;
	void				*mesh_ibo; 
	uint32_t			 mesh_ibo_items;
};

/*
 * Synchronization
 * For voxel access, you must use open/close functions below.
 *
 * Volatile variables here are signals used by multiple threads, with no locks.
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


int shadow_sample( 
	struct ChunkSet *set, 
	uint32_t *ws 
);
/*
uint8_t sample_shadow_c( 
	struct ChunkSet *set, 
	struct ChunkMD *c, 
	int16_t *ws 
);*/
void shadow_init( 
	struct ChunkSet *set
);
void shadow_place_update(
	struct ChunkSet *set,
	uint32_t *ws
);
void shadow_break_update(
	struct ChunkSet *set,
	uint32_t *ws
);



#endif
