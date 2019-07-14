#define __FILENAME__ "chunkset/mesher.c"
#include "chunkset/mesher.h"
#include "chunkset.h"
#include "chunkset/edit.h"
#include "mem.h"
#include "event.h"
#include "ctx.h"

#include "cpp/noise.h"

#include <string.h>
#include <omp.h>

#include "rle.h"

// When the file starts with look up tables, you know its going to be fun!

// These are vertices of each face of a cube
const float lutv[72] = {
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 0.0f,

	0.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,

	1.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,

	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 1.0f,

	0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 0.0f,

	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
};
// These are indecises to sample the vertices
const unsigned short luti[12] = {
	0, 3, 1,
	2, 1, 3,
	// Inverted
	3, 2, 0,
	1, 0, 2
};

float *work_buf[128] = {NULL};


// TODO: make these better :D
void sample_ao(
	struct ChunkSet *set,
	struct ChunkMD	*c,
	uint16_t 		*AIR,
	uint8_t			face,
	uint8_t 		*ao,
	uint8_t 		*aoc
){
	uint8_t uv[2];
	uint16_t O[3];
	uint32_t d = 0;

	for( int a = 0; a < 3; a++ ){
		if( a == face ) continue;
		uv[d]=a;

		memcpy( O, AIR, sizeof(O) );

		O[a]++;
		ao[d+2] = !!c->voxels[ flatten1( O, set->root_bitw )];

		memcpy( O, AIR, sizeof(O) );
		O[a]--;
		ao[d] =  !!c->voxels[ flatten1( O, set->root_bitw )];

		d++;
	}

	memcpy( O, AIR, sizeof(O) );
	O[uv[0]]--;
	O[uv[1]]--;
	aoc[0] = !!c->voxels[ flatten1( O, set->root_bitw ) ];
	memcpy( O, AIR, sizeof(O) );
	O[uv[0]]++;
	O[uv[1]]--;
	aoc[1] = !!c->voxels[ flatten1( O, set->root_bitw )];
	memcpy( O, AIR, sizeof(O) );
	O[uv[0]]++;
	O[uv[1]]++;
	aoc[2] = !!c->voxels[ flatten1( O, set->root_bitw )];
	memcpy( O, AIR, sizeof(O) );
	O[uv[0]]--;
	O[uv[1]]++;
	aoc[3] = !!c->voxels[ flatten1( O, set->root_bitw )];
}


void sample_ao_border(
	struct ChunkSet *set,
	struct ChunkMD	*c,
	uint16_t 		*AIR,
	uint8_t			face,
	uint8_t 		*ao,
	uint8_t 		*aoc
){
	uint8_t uv[2];

	uint32_t ws[3];
	for (int i = 0; i < 3; ++i){
		ws[i] = (c->offset[i]<<set->root_bitw)+AIR[i];
	}

	uint32_t O[3];


	uint32_t d = 0;

	for( int a = 0; a < 3; a++ ){
		if( a == face ) continue;
		uv[d]=a;

		memcpy( O, ws, sizeof(O) );

		O[a]++;
		ao[d+2] = !!chunkset_edit_read(set, O);

		memcpy( O, ws, sizeof(O) );
		O[a]--;
		ao[d] = !!chunkset_edit_read(set, O);

		d++;
	}

	memcpy( O, ws, sizeof(O) );
	O[uv[0]]--;
	O[uv[1]]--;
	aoc[0] = !!chunkset_edit_read(set, O);
	memcpy( O, ws, sizeof(O) );
	O[uv[0]]++;
	O[uv[1]]--;
	aoc[1] = !!chunkset_edit_read(set, O);
	memcpy( O, ws, sizeof(O) );
	O[uv[0]]++;
	O[uv[1]]++;
	aoc[2] = !!chunkset_edit_read(set, O);
	memcpy( O, ws, sizeof(O) );
	O[uv[0]]--;
	O[uv[1]]++;
	aoc[3] = !!chunkset_edit_read(set, O);
}

int chunk_unsafe(uint16_t *C, uint16_t R){
	return (
		C[0] == 0 ||
		C[1] == 0 ||
		C[2] == 0 ||
		C[0] == R ||
		C[1] == R ||
		C[2] == R
	);
}

void chunk_make_mesh(
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	int16_t *geometry,
	uint32_t*geometry_items,
	uint32_t*index,
	uint32_t*index_items
){
	// Dereferense the structs for 20% speedup
	struct ChunkSet set = *setp;		
	struct ChunkMD c = *cp;

	uint16_t cvec[3];
	memcpy( cvec, c.offset, sizeof(cvec) ); 

	struct ChunkMD nc[3];

	for( int i = 0; i < 3; i++ ) {
		cvec[i]++;
		if( cvec[i]+1 > set.max[i]  )
			nc[i] = *set.null_chunk;
		else{
			struct ChunkMD *_c = &set.chunks[ flatten3( cvec, set.max_bitw )];
			chunk_open_ro( setp, _c );
			nc[i] = *_c;
			chunk_close_ro( setp, _c );
		}
		cvec[i]--;
	}

	uint32_t v = 0;

	uint32_t ibo_len = 0;
	uint32_t vertices = 0;


	// CULLER
	uint16_t r = set.root-1;
	uint16_t AC[3];
	uint32_t Ai = 0;
	for( AC[2] = 0; AC[2] < set.root; AC[2]++ )
	for( AC[1] = 0; AC[1] < set.root; AC[1]++ )
	for( AC[0] = 0; AC[0] < set.root; AC[0]++ )
	{
		Voxel A = c.voxels[Ai];
		uint32_t shift = 0;
		for( int i = 0; i < 3; i++ ){
			Voxel B;
			if( AC[i] == set.root-1 )
				B = nc[i].voxels[ Ai - (r << shift) ];
			else
				B = c.voxels[ Ai + (1 << shift) ];

			shift += set.root_bitw;

			if ( !A == !B  ) continue;

			// PUSH TO MASK
			uint16_t AIR[3];
			memcpy( AIR, AC, sizeof(AIR) );
			AIR[i] += (B==0);
			uint16_t BLOCK[3];
			memcpy( BLOCK, AC, sizeof(AIR) );
			BLOCK[i] += (A==0);

			// AO START

			uint8_t aon[4] = {0};
			uint8_t aoc[4] = {0};

			if(!chunk_unsafe(AC, set.root-1))
				sample_ao(			&set, &c, AIR, i, aon, aoc);
			else
				sample_ao_border(	&set, &c, AIR, i, aon, aoc);

			uint8_t vertex_ao[4];
			if( i == 0 ){
				vertex_ao[0] = (aon[0]+aon[1])|aoc[0];
				vertex_ao[3] = (aon[1]+aon[2])|aoc[1];
				vertex_ao[2] = (aon[2]+aon[3])|aoc[2];
				vertex_ao[1] = (aon[3]+aon[0])|aoc[3];
			} else if( i == 1 ) { 
				vertex_ao[0] = (aon[0]+aon[1])|aoc[0];
				vertex_ao[1] = (aon[1]+aon[2])|aoc[1];
				vertex_ao[2] = (aon[2]+aon[3])|aoc[2];
				vertex_ao[3] = (aon[3]+aon[0])|aoc[3];
			} else {
				vertex_ao[1] = (aon[0]+aon[1])|aoc[0];
				vertex_ao[0] = (aon[1]+aon[2])|aoc[1];
				vertex_ao[3] = (aon[2]+aon[3])|aoc[2];
				vertex_ao[2] = (aon[3]+aon[0])|aoc[3];
			}

			uint8_t invert=0;
			if( vertex_ao[0]+vertex_ao[2] < vertex_ao[1]+vertex_ao[3] ) 
				invert = 6;


			uint32_t ws[3];
			for (int i = 0; i < 3; ++i)
				ws[i] = (c.offset[i] << set.root_bitw) + BLOCK[i];

			uint16_t SHADOW = sample_shadow(setp, ws)<<15;

			ws[i] -= (A==0);

			int face = i;
			for( int vertex = 0; vertex < 4; vertex++ ){
				for( int item = 0; item < 3; item++  ){
					geometry[v++] = ( ws[item]
						+ lutv[ face*12 + vertex*3 + item ]
					);
				}
				geometry[v++] = 
					(A|B) | 
					( vertex_ao[vertex]  << 6) |
					((face + (3*(A==0))) << 8) |	// Normal
					SHADOW
				; 
			}
			for( int vertex = 0; vertex < 6; vertex++ ){
				index[ibo_len++] = vertices+luti[vertex+invert];
			}
			vertices+=4;


		}
		Ai++;
	}
	*geometry_items = v;
	*index_items = ibo_len;
	return;
}



uint32_t flatten1_no_po2( uint16_t *l, uint16_t root ){

	uint32_t i = l[2];
	i*=root;
	i+=l[1];
	i*=root;
	i+=l[0];
	return i;

}


void chunk_make_mask(
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	Voxel *mask
){
	struct ChunkSet set = *setp;		
	struct ChunkMD c = *cp;

	uint16_t cvec[3];
	memcpy( cvec, c.offset, sizeof(cvec) ); 

	struct ChunkMD nc[3];

	for( int i = 0; i < 3; i++ ) {
		cvec[i]++;
		if( cvec[i]+1 > set.max[i]  )
			nc[i] = *set.null_chunk;
		else{
			struct ChunkMD *_c = &set.chunks[ flatten3( cvec, set.max_bitw )];
			chunk_open_ro( setp, _c );
			chunk_close_ro( setp, _c );
			nc[i] = *_c;
		}
		cvec[i]--;
	}

	uint16_t r = set.root-1;
	uint16_t AC[3];
	uint32_t Ai = 0;
	for( AC[2] = 0; AC[2] < set.root; AC[2]++ )
	for( AC[1] = 0; AC[1] < set.root; AC[1]++ )
	for( AC[0] = 0; AC[0] < set.root; AC[0]++ )
	{
		Voxel A = c.voxels[Ai];
		uint32_t shift = 0;
		for( int i = 0; i < 3; i++ ){
			Voxel B;
			if( AC[i]+1 == set.root )
				B = nc[i].voxels[ Ai - (r << shift) ];
			else
				B = c.voxels[ Ai + (1 << shift) ];

			shift += set.root_bitw;

			if ( !A == !B  ) continue;

			// Face visible, store result
			uint16_t MC[3];
			memcpy( MC, AC, sizeof(MC) );
			MC[i] += (A==0);
			//uint32_t Mi = flatten1( MC, set.root_bitw+1 );
			//uint32_t Mi = flatten1_no_po2( MC, 1 << (set.root_bitw+1) );
			uint32_t Mi = flatten1_no_po2( MC, (1<<set.root_bitw)+1 );
			mask[Mi] = A|B;
		}
		Ai++;
	}

	return;
}



void chunk_mask_downsample(
	struct ChunkSet *setp,
	int8_t level,
	Voxel *mask,
	Voxel *work
){
	struct ChunkSet set = *setp;		

	//uint16_t r = (set.root<<level) + 1;
	uint16_t r = (set.root) + 1;
	uint16_t AC[3];

	uint16_t range = (set.root>>(level-1))+1;

	// Loop thru the mask
	for( AC[2] = 0; AC[2] < range; AC[2]++ )
	for( AC[1] = 0; AC[1] < range; AC[1]++ )
	for( AC[0] = 0; AC[0] < range; AC[0]++ )
	{
		uint32_t Mi = flatten1_no_po2( AC, r );
		if( mask[Mi] == 0 ) continue;

		uint16_t MC[3];
		memcpy( MC, AC, sizeof(MC) );
		for (int i = 0; i < 3; ++i){
			MC[i]>>=(1);
		}

		uint32_t Wi = flatten1_no_po2( MC, r );
		//uint32_t Wi = flatten1( MC, set.root_bitw+1 );
		work[ Wi ] = mask[ Mi ];
		Mi++;
	}

	return;
}



void chunk_make_splatlist(
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	uint8_t level,
	Voxel *mask,
	int16_t *geometry,
	uint32_t*geometry_items
){
	// Dereferense the structures for 20% speedup
	struct ChunkSet set = *setp;		
	struct ChunkMD c = *cp;

	//uint8_t lod = c.lod-1;
	
	uint16_t AC[3];

	uint32_t v = *geometry_items;
	uint16_t range = (set.root>>(level))+1;
	// Loop thru the mask
	for( AC[2] = 0; AC[2] < range; AC[2]++ )
	for( AC[1] = 0; AC[1] < range; AC[1]++ )
	for( AC[0] = 0; AC[0] < range; AC[0]++ )
	{
		//uint32_t Mi = flatten1( AC, set.root_bitw+1 );
		uint32_t Mi = flatten1_no_po2( AC, (1<<set.root_bitw)+1 );
		if( mask[Mi] == 0 ) continue;

		uint32_t ws[3];
		for( int j = 0; j < 3; j++  ){
			ws[j] = (c.offset[j] << set.root_bitw) + (AC[j]<<level);
			geometry[v++] = ( ws[j] );
		}
		if(level){
			ws[0]+= 1<<(level);
			ws[1]+= 1<<(level);
			ws[2]+= 1<<(level);
		}
		geometry[v++] = mask[Mi] | ((sample_shadow( setp, ws )<<6));
	}
	*geometry_items += v;

	return;
}





