#define __FILENAME__ "chunkset/rle.c"
#include "chunkset/rle.h"
#include "string.h"
#include "mem.h"
#include "event.h"


#include <stdint.h>
#include <pthread.h>

#include "chunkset.h"

/*
	New RLE format

	32 bit ints where
	8 bit data, 24 bit run length

	Makes it possible to encode single voxel chunks with a single integer.
	256^3 is the maximum chunk size for this
	cbrt(2**24) = 256 
*/


#define BITMASK 0xFFFFFF
#define BITSHIFT 24

// Mutex unused atm
pthread_mutex_t rle_work_mutex = PTHREAD_MUTEX_INITIALIZER; 

uint32_t *rle_work_buf = NULL;
uint32_t  rle_work_buf_len = 0;

int rle_init(){

	if( pthread_mutex_init( &rle_work_mutex, NULL ) != 0 ){
		logf_error( "Mutex init failed" );
		return 1;
	}

	return 0;
}

Voxel *rle_compress( Voxel* data, uint32_t length )
{

	rle_work_buf_len = length;;

	uint32_t *_work_buf = mem_alloc( rle_work_buf_len );


	uint32_t count = 1;
	Voxel last = data[0];
	Voxel current;
	uint32_t b = 0;

	for( uint32_t i = 1; i < length; i++ ){
		
		current = data[i];
		

		if( current == last && count < (uint32_t)BITMASK){
			count++;
		}else{

			_work_buf[b++] = count | (last << BITSHIFT);

			count = 1;
			last = current;
		}
		

	}
		
	_work_buf[b++] = count | (last << BITSHIFT);

	_work_buf[b++] = 0;

	Voxel *buf = mem_alloc( b*sizeof(uint32_t) );

	memcpy( buf, _work_buf, b*sizeof(uint32_t) );

	mem_free( _work_buf );


	return buf;
}


Voxel *rle_decompress( void *vdata  )
{

	uint32_t *data = (uint32_t*)vdata;

	Voxel *_work_buf = mem_alloc( rle_work_buf_len );

	uint32_t i = 0;
	uint32_t b = 0;
	// Loop while null
	do {

		uint32_t count = (data[i] & BITMASK);
		Voxel item = data[i] >> BITSHIFT;

		memset(_work_buf+b, item, count);
		b+=count;
		
	} while( data[++i] );
	
	Voxel *buf = mem_alloc( b*sizeof(Voxel) );

	memcpy( buf, _work_buf, b*sizeof(Voxel) );
	mem_free( _work_buf );

	return buf;
}

