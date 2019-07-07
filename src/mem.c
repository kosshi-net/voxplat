#define __FILENAME__ "mem.h"

#include "mem.h"
#include "event.h"
#include <stdlib.h>
#include <string.h>

#include "shell.h"

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




#define HEADSIZE sizeof(struct MemoryNode)

void *mem;

struct MemoryNode
{
	struct MemoryNode 	*prev;
	struct MemoryNode 	*next;
	size_t 	 			size;
	int 	 			free;
};



void mem_print_block(struct MemoryNode *mbh){
	printf("-----\n");
	printf("  prev %p\n", 	mbh->prev );
	printf("  this %p\n", 	mbh );
	printf("  next %p\n", 	mbh->next );
	printf("  size %li\n",	mbh->size );
	printf("  free %i\n", 	mbh->free );

	// Find the label
	for( int i = 0; i < at_index; i++  ){
		if( alloc_table[i].p == (mbh+1) )
			printf( "%lu K, %s\n", alloc_table[i].size/1024, alloc_table[i].label);
		
	}


}

void mem_block_print_debug(){
	struct MemoryNode *mbh = mem;

	printf("Traversing by pointers\n");
	while(1){
		mem_print_block(mbh);
		if( mbh->next == NULL ) break;
		mbh = mbh->next;
	}
}



void *mem_block_alloc(size_t bytes){

	// Aligment
	if( bytes % HEADSIZE != 0 )
		bytes += HEADSIZE - ( bytes % HEADSIZE );

	struct MemoryNode *best = NULL;
	struct MemoryNode *node = mem;

	// Find the smallest fitting block, and name it best
	while(1){
		if( node->free == 0 || node->size < bytes+HEADSIZE ) {
			goto next;
		}

		if( !best ) 
			best = node;
		else if ( node->size < best->size ) 
			best = node;
		
		next:

		if( node->next == NULL ) 
			break;
		node = node->next;
	}

	if(best == NULL) {
		printf("Cannot allocate\n");
		mem_block_print_debug();
		printf("Cannot allocate\n");
		return NULL;
	}

	// Is there enough room to split this node?
	// Smallest allocatable block is HEADSIZE, add that
	if( best->size < bytes+HEADSIZE+HEADSIZE ){
		best->free = 0;
		return best+1;
	}

	// Split the node

	size_t old_size = best->size;
	best->size = bytes;
	best->free = 0;

	// Create a new node
	struct MemoryNode *new = best + (bytes / HEADSIZE) + 1;

	new->prev = best;
	new->next = best->next;
	best->next = new;

	new->size = old_size - best->size - HEADSIZE;
	new->free = 1;

	//memset( (void*)(best+1), 0x00, best->size );

	return best + 1;
}


void mem_block_free(void *block){
	struct MemoryNode *node = (void*)block - HEADSIZE;

	node->free = 1;

	// Jump to the first free node in the chain
	while( node->prev && node->prev->free == 1 ){
		node = node->prev;
	}
	// Merge all the next free nodes
	while( node->next && node->next->free == 1 ){

		node->size += node->next->size + HEADSIZE;

		struct MemoryNode *del = node->next;
		node->next = del->next;

		if(del->next)
			del->next->prev = node;
	}


}


void command_listdebug( int argc, char **argv ){
	mem_block_print_debug();
}

int mem_init(size_t heap_size){
	shell_bind_command("mem_list", 		&command_listdebug);

	printf("Init memory\n");
	printf("Head size %li\n", HEADSIZE);

	mem = malloc( heap_size );

	struct MemoryNode *mem_block_head = mem;

	mem_block_head->prev = NULL;
	mem_block_head->next = NULL;
	mem_block_head->size = heap_size - HEADSIZE;
	mem_block_head->free = 1;

	return 0;
}



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

	//void* p = malloc( bytes );
	void* p = mem_block_alloc(bytes);
	memset( p, 0, bytes );

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
	
	//void* p = calloc( bytes, 1 );
	//void* p = malloc( bytes );
	void* p = mem_block_alloc(bytes);
	memset( p, 0, bytes );

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
		
	//free( p );
	mem_block_free(p);

//	logf_info("Freed %lu", alloc_table[_at_i].size);

	total_bytes -= alloc_table[_at_i].size;

	at_index--;	
	alloc_table[_at_i].p 	= alloc_table[at_index].p;
	alloc_table[_at_i].size = alloc_table[at_index].size;

	pthread_mutex_unlock( &mem_mtx );
}
