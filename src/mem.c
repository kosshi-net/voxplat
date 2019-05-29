#define __FILENAME__ "mem.h"

#include "mem.h"
#include "event.h"
#include <stdlib.h>

#include <pthread.h>

// Alloc wrapper, keeps track of memory usage
// Todo: your own malloc

#define ALLOC_TABLE_SIZE (1024*512)
#define MAX_DYNAMIC_BYTES (1024*1024*1024*(size_t)4)

pthread_mutex_t mem_mtx = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
	void*  p;
	char* label;
	size_t size;
} Allocation;


Allocation alloc_table[ALLOC_TABLE_SIZE];
int at_index = 0;

// total allocated, just for debugging
size_t total_bytes = 0;

void mem_log_debug(){

	logf_info( "MEMORY DEBUG \n" 
			"Dynamic: %lu b (%lu k)\n"
			"Fragments: %i", 

		total_bytes, total_bytes/1024, at_index  );
	
	for( int i = 0; i < at_index; i++  )
		logf_info( "%lu kb, %s", alloc_table[i].size/1024, alloc_table[i].label);


}

size_t mem_dynamic_allocation(void){
	return total_bytes;
}



void* _mem_alloc( size_t bytes, char*label ){
	pthread_mutex_lock( &mem_mtx );

	if( at_index+1 >= ALLOC_TABLE_SIZE ){
		logf_error( "Allocation failure! Dynamic fragment count exceeded!" );
		panic();
	}
	if( total_bytes + bytes > MAX_DYNAMIC_BYTES  ){
		logf_error( "Allocation failure! Dynamic memory limit exceeded!" );
		panic();
	}

	void* p = malloc( bytes );

	if( p == NULL  ) {
		logf_error( "Allocation failure! %lu b %s", bytes, label  );
		panic();
	}
	alloc_table[at_index].p = p;
	alloc_table[at_index].size = bytes;
	alloc_table[at_index].label = label;
	at_index++;	
	total_bytes += bytes;

	pthread_mutex_unlock( &mem_mtx );
	return p; 
}

void* _mem_calloc( size_t bytes, char*label ){

	pthread_mutex_lock( &mem_mtx );

	if( at_index+1 >= ALLOC_TABLE_SIZE ){
		logf_error( "Allocation failure! Dynamic fragment count exceeded!" );
		panic();
	}
	if( total_bytes + bytes > MAX_DYNAMIC_BYTES  ){
		logf_error( "Allocation failure! Dynamic memory limit exceeded!" );
		panic();
	}
	void* p = calloc( bytes, 1 );

	if( p == NULL  ) {
		logf_error( "Allocation failure! %lu b %s", bytes, label  );
		panic();
	}

	alloc_table[at_index].p = p;
	alloc_table[at_index].size = bytes;
	alloc_table[at_index].label = label;
	at_index++;
	total_bytes += bytes;

	pthread_mutex_unlock( &mem_mtx );

	return p;
}

void _mem_free( void* p, char*label){
	
	pthread_mutex_lock( &mem_mtx );

	if( p == NULL  ){
		logf_error(
			"Deallocation error:\n" 
			"Tried to deallocate a NULL pointer. Source:\n%s",
			
			label
		);
		panic();
		return;
	}

	// Find the index of the pointer wished to deallocate
	int _at_i = -1;
	for( int i = 0; i < at_index; i++  ){
		if( alloc_table[i].p == p  ) {
			_at_i = i;
			break;
		}
	}
	
		
	if( _at_i == -1  ) { 
		logf_error(
			"Deallocation error:\n" 
			"Unknown address, double free? Source:\n%s",
			
			label
		);
		panic();
		return;
	}
	// Decrement index and move the last item to current place
		
	free( p );

//	logf_info("Freed %lu", alloc_table[_at_i].size);

	total_bytes -= alloc_table[_at_i].size;

	at_index--;	
	alloc_table[_at_i].p 	= alloc_table[at_index].p;
	alloc_table[_at_i].size = alloc_table[at_index].size;

	pthread_mutex_unlock( &mem_mtx );
}
