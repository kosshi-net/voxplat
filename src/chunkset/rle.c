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



pthread_mutex_t rle_work_mutex = PTHREAD_MUTEX_INITIALIZER;

Voxel *rle_work_buf = NULL;
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
	//logf_info("Compress");
	pthread_mutex_lock( &rle_work_mutex  );

	
	
	if( rle_work_buf_len < length  ){
		logf_info( "Resizing working buffer from %i to %i", 
			rle_work_buf_len, 
			length 
		);
		if(rle_work_buf != NULL) mem_free(rle_work_buf);
		rle_work_buf_len = length;

		rle_work_buf = mem_alloc( rle_work_buf_len );
	}
	

	Voxel last = data[0];
	Voxel count = 1;
	Voxel current;
	uint32_t b = 0;

	for( uint32_t i = 1; i < length; i++ ){
		
		current = data[i];
		
		if( current == last && count < 0xFF){
			count++;
		}else{
			
			// write
			
			rle_work_buf[b++] = count;
			rle_work_buf[b++] = last;

			count = 1;
			last = current;
		}
		

	}
	rle_work_buf[b++] = count;
	rle_work_buf[b++] = last;
	rle_work_buf[b++] = 0;
	rle_work_buf[b++] = 0;

	Voxel *buf = mem_alloc( b );

	memcpy( buf, rle_work_buf, b );

	pthread_mutex_unlock( &rle_work_mutex  );

	return buf;
}


Voxel *rle_decompress( Voxel *data  )
{
	//logf_info("Decompress");
	pthread_mutex_lock( &rle_work_mutex  );

	Voxel count = 0;

	uint32_t i = 0;
	uint32_t b = 0;
	// Loop while null
	while( (count=data[i]) != 0 ){
		i+=2;
		
		for( Voxel j = 0; j < count; j++  ){
			rle_work_buf[b++] = data[i-1];
		}

	}
	
	Voxel *buf = mem_alloc( b*sizeof(Voxel) );

	memcpy( buf, rle_work_buf, b*sizeof(Voxel) );

	pthread_mutex_unlock( &rle_work_mutex  );

	return buf;
}



