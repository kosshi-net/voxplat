#define __FILENAME__ "chunkset/rle.c"
#include "chunkset/rle.h"
#include "string.h"
#include "mem.h"
#include "event.h"


#include <stdint.h>
#include <pthread.h>

#include "chunkset.h"

/*
 * RLE
 * 
 * NUM DATA NUM DATA NUM DATA NULL
 * 
 * Terminate when NUM is 0 
 *
 * Note:
 * When changing voxel byte size in chunkset.h, some edits here may be necessary
 */


/*
	New RLE format
	RLE Header:

	INT Total including header 
	INT Length of actual RLE data

	32 bit ints where
	8 bit data, 24 bit run length

*/


#define BITMASK 0xFFFFFF
#define BITSHIFT 24


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
	/*
	//logf_info("Compress");
	pthread_mutex_lock( &rle_work_mutex  );

	if( rle_work_buf_len < length  ){
		logf_info( "Resizing working buffer from %i to %i", 
			rle_work_buf_len, 
			length 
		);
		if(rle_work_buf != NULL) mem_free(rle_work_buf);
		rle_work_buf_len = length;

		//rle_work_buf = mem_alloc( rle_work_buf_len*sizeof(uint32_t) );
	}
	*/
	//uint32_t work_buf 

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

	//pthread_mutex_unlock( &rle_work_mutex  );

	return buf;
}


Voxel *rle_decompress( void *vdata  )
{
	//Voxel *_work_buf = (Voxel*)rle_work_buf;

	uint32_t *data = (uint32_t*)vdata;

	Voxel *_work_buf = mem_alloc( rle_work_buf_len );

	//logf_info("Decompress");
	//pthread_mutex_lock( &rle_work_mutex  );

	uint32_t i = 0;
	uint32_t b = 0;
	// Loop while null
	do {

		uint32_t count = (data[i] & BITMASK);
		Voxel item = data[i] >> BITSHIFT;

		memset(_work_buf+b, item, count);
		b+=count;
		//for (int j = 0; j < count; ++j)
		//	_work_buf[b++] = item;

		
	} while( data[++i] );
	
	Voxel *buf = mem_alloc( b*sizeof(Voxel) );

	memcpy( buf, _work_buf, b*sizeof(Voxel) );
	mem_free( _work_buf );
	//pthread_mutex_unlock( &rle_work_mutex  );

	return buf;
}

/*
size_t rle_size ( uint32_t rle_data ){
	int i = 0;
	while( rle_data[i] )
}
*/


Voxel chunk_read_index( Voxel *data, uint32_t index ){

	uint32_t count = 0;
	uint32_t i = 0;

	do {
		count += data[i] & BITMASK;
		if( count < index ) return (data[i-1]>>BITSHIFT);
	} while ( data[++i] );

	return 0;
	
}

/*
Voxel chunk_write_index( Voxel *data, uint32_t index, uint32_t data ){

	uint32_t count = 0;
	uint32_t i = 0;

	do {
		count += data[i] & BITMASK;
		if( count < index ) return (data[i-1]>>BITSHIFT);
	} while ( data[++i] );

	return 0;
	
}
*/