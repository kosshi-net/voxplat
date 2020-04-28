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

// Vertex buffer LUT. Each 3*4 block represents a face of a cube
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

	// Below actually unused atm
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

// Index buffer LUT
const unsigned short luti[24] = {
	0, 3, 1,
	2, 1, 3,
	// AO rotation
	3, 2, 0,
	1, 0, 2,

	// And the same thing but x and z swapped for backside culling and rotation
	1, 3, 0, 
	3, 1, 2, 
	0, 2, 3, 
	2, 0, 1

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
			// Visible, make mesh

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

			uint8_t rotate=0;  
			if( vertex_ao[0]+vertex_ao[2] < vertex_ao[1]+vertex_ao[3] ) 
				rotate = 6;


			uint32_t ws[3];
			for (int i = 0; i < 3; ++i)
				ws[i] = (c.offset[i] << set.root_bitw) + BLOCK[i];

			uint16_t shadow = shadow_sample(setp, ws);
			uint8_t  normal = (A==0);

			ws[i] -= normal;

			int face = i;
			for( int vertex = 0; vertex < 4; vertex++ ){

				// Push coordinates
				for( int item = 0; item < 3; item++  ){
					geometry[v++] = ( ws[item]
						+ lutv[ face*12 + vertex*3 + item ]
					);
				}

				// Push voxel data
				geometry[v++] = 
					(A|B) | 
					( vertex_ao[vertex]  << 6) |
					((face + (3*normal)) << 8) |
					(shadow              <<15) ;
			}

			// Fill index buffer
			for( int vertex = 0; vertex < 6; vertex++ ){
				index[ibo_len++] = vertices + 
					luti[ (normal*12) + vertex + rotate ];
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
	struct ChunkMD *ncp[3] = {NULL};

	for( int i = 0; i < 3; i++ ) {
		cvec[i]++;
		if( cvec[i]+1 > set.max[i]  )
			nc[i] = *set.null_chunk;
		else{
			struct ChunkMD *_c = &set.chunks[ flatten3( cvec, set.max_bitw )];
			//chunk_open_ro( setp, _c );
			//chunk_close_ro( setp, _c );
			ncp[i] = _c;
		}
		cvec[i]--;
	}

	if( cp->rle == setp->null_chunk->rle ){
		for (int i = 0; i < 3; ++i){
			if( nc[i].rle != setp->null_chunk->rle ) goto no_null;
		}
		return;
	}
	no_null:;

	for( int i = 0; i < 3; i++ )
		if(ncp[i]){
			chunk_open_ro( setp, ncp[i] );	
			nc[i] = *ncp[i];
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

	for( int i = 0; i < 3; i++ )
		if(ncp[i])
			chunk_close_ro( setp, ncp[i] );	


	return;
}



void chunk_mask_downsample(
	struct ChunkSet *setp,
	int8_t level,
	Voxel *mask,
	Voxel *work
){
	struct ChunkSet set = *setp;		

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
	struct ChunkSet set = *setp;		
	struct ChunkMD c = *cp;

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
			ws[0] += 1<<(level);
			ws[1] += 1<<(level);
			ws[2] += 1<<(level);
		}
		geometry[v++] = mask[Mi] | ((shadow_sample( setp, ws )<<6));
	}
	*geometry_items += v;

	return;
}





/*
void _mkmesh(
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	int16_t *geometry,
	uint32_t*geometry_items,
	uint32_t*index,
	uint32_t*index_items
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
			nc[i] = *_c;
			chunk_close_ro( setp, _c );
		}
		cvec[i]--;
	}

	uint32_t v = 0;

	// root = cuberoot of volume
	// root_bitw = __builtin_ctz(root)
	// c = chunk, nc neighboring chunks

	uint32_t r = root-1;
	uint32_t Ap[3]; 
	uint32_t Ai = 0;
	for( Ap[2] = 0; Ap[2] < root; Ap[2]++ )
	for( Ap[1] = 0; Ap[1] < root; Ap[1]++ )
	for( Ap[0] = 0; Ap[0] < root; Ap[0]++ )
	{
		uint8_t A = c.volume[Ai];
		uint32_t shift = 0;
		for( int i = 0; i < 3; i++ ){
			uint8_t B;
			if( Ap[i] == r )
				B = nc[i].volume[ Ai - (r << shift) ];
			else
				B = c.volume[ Ai + (1 << shift) ];
			shift += root_bitw;
			if ( !A == !B  ) continue;
			// Face visible, do something
		}
		Ai++;
	}

}

*/


/*
void chunk_make_mesh(
	struct ChunkSet *setp,
	struct ChunkMD *cp,
	int16_t *geometry,
	uint32_t*geometry_items,
	uint32_t*index,
	uint32_t*index_items
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
			nc[i] = *_c;
			chunk_close_ro( setp, _c );
		}
		cvec[i]--;
	}


	
	uint32_t x_ints = set.root / 32;

	uint32_t cullmask[
		(x_ints  +1) * 
		(set.root+1) *
		(set.root+1)
	];
	
	uint32_t *cm = cullmask;

	// Load
	uint32_t Ac[3];
	for( Ac[2] = 0; Ac[2] < set.root; Ac[2]++ )	
	for( Ac[1] = 0; Ac[1] < set.root; Ac[1]++ )	{

		uint32_t min = flatten1( setp, Ac );
		uint32_t max = min+set.root;
		
		uint32_t mask = 0;

		for( int _i = 0; _i <  )
		for( int i = min; i < max; i++ ){
			
			mask <<= 1;
			mask  |= c.voxels[i];

		}
	
		

	}




	uint32_t v = 0;

	uint32_t ibo_len = 0;
	uint32_t vertices = 0;

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
			// Visible, make mesh

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

			uint8_t rotate=0;  
			if( vertex_ao[0]+vertex_ao[2] < vertex_ao[1]+vertex_ao[3] ) 
				rotate = 6;


			uint32_t ws[3];
			for (int i = 0; i < 3; ++i)
				ws[i] = (c.offset[i] << set.root_bitw) + BLOCK[i];

			uint16_t shadow = shadow_sample(setp, ws);
			uint8_t  normal = (A==0);

			ws[i] -= normal;

			int face = i;
			for( int vertex = 0; vertex < 4; vertex++ ){

				// Push coordinates
				for( int item = 0; item < 3; item++  ){
					geometry[v++] = ( ws[item]
						+ lutv[ face*12 + vertex*3 + item ]
					);
				}

				// Push voxel data
				geometry[v++] = 
					(A|B) | 
					( vertex_ao[vertex]  << 6) |
					((face + (3*normal)) << 8) |
					(shadow              <<15) ;
			}

			// Fill index buffer
			for( int vertex = 0; vertex < 6; vertex++ ){
				index[ibo_len++] = vertices + 
					luti[ (normal*12) + vertex + rotate ];
			}
			vertices+=4;
		}

		Ai++;
	}
	*geometry_items = v;
	*index_items = ibo_len;
	return;
}
*/
