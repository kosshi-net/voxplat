#define __FILENAME__ "mem.h"

#include "mem.h"
#include "event.h"
#include <stdlib.h>
#include <string.h>

#include "shell.h"

#include "ctx.h" // Only for ctx_time

#include <pthread.h>

/*
 * Memory allocating and debugging wrapper
 * 
 */

#define ALLOC_TABLE_SIZE (1024*512)


/* Uncomment only for debugging */
//#define USE_SYSTEM_ALLOC
/* If a slight buffer overrun doesn't still segfault, fill the alloc function
 * with mem_print_block, it will print what line allocated the previous block
 * that corrupt your links
 * Todo: dumb function names
 */

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
size_t mem_size;


struct MemoryNode
{
	struct MemoryNode 	*prev;
	struct MemoryNode 	*next;
	size_t 	 			size;
	int 	 			free;
};


struct MemoryNode *mem_tail;
struct MemoryNode *mem_first_free;


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


void integrity_check(char*source){

	struct MemoryNode *node = mem;

	int last_was_free = node->free;

	while( node->next ){

		if( last_was_free && node->free ){
			printf("Multiple free blocks a row %s\n", source);
			exit(1);
		}
		last_was_free = node->free;

		if( node->next->prev != node ) {
			printf("Bad links %s\n", source);
			printf("me %p\n", node);
			printf("me->next->prev %p\n", node->next->prev);
			exit(1);
		}

		node = node->next;
	}
}


void mem_corrupt_block( struct MemoryNode *node ){
	mem_print_block( node );
	mem_print_block( node->next );
	panic();
}


//#define BEST_FIT
void *mem_block_alloc(size_t bytes){

	int align = HEADSIZE-1;
	bytes = (bytes + align) &~ align;

	struct MemoryNode *best = NULL;

	struct MemoryNode *node = (mem_first_free) ? mem_first_free : mem;

	#ifdef BEST_FIT
	
	do { // Find the smallest fitting block
		if( node->free && node->size >= bytes  
		&&  (!best || node->size < best->size )
		) 	best = node;
	} while( (node=node->next) );

	#else

	do { // Find first fitting block
		if( node->free && node->size >= bytes ){
			best = node;
			break;
		}

		if( node->next && node->next->free > 1 ) mem_corrupt_block(node);

	} while( (node=node->next) );

	#endif

	if(best == NULL) return NULL;
	best->free = 0;
	if(mem_first_free == best) mem_first_free = NULL;
	// if not enough room to split the node
	if( best->size < bytes+HEADSIZE+HEADSIZE ) return best+1;
	// Create new node
	size_t old_size = best->size;
	best->size = bytes;
	struct MemoryNode *new = best + (bytes / HEADSIZE) + 1;
	new->prev = best;
	new->next = best->next;
	if(new->next) new->next->prev = new;
	new->size = old_size - best->size - HEADSIZE;
	best->next = new;
	new->free = 1;
	if(mem_first_free == NULL) mem_first_free = new;
	return best + 1;
}

void mem_block_free(void *address){
	struct MemoryNode *node = address-HEADSIZE;
	node->free = 1;

	if( node->prev && node->prev->free ) node = node->prev;	

	while( node->next && node->next->free ){ // Merge all the next free nodes.
		node->size += node->next->size + HEADSIZE;

		struct MemoryNode *del = node->next;
		node->next = del->next;

		if(node->next) node->next->prev = node;
	}

	if(mem_first_free == NULL || mem_first_free > node) mem_first_free = node;
}


void command_memory_labels( int argc, char **argv ){
	mem_log_debug();
}

#define CHART_SIZE 80

void command_memory_status( int argc, char **argv ){

	struct MemoryNode *node = mem_tail;
	uint32_t nodes = 0;
	size_t memory_free_total = 0;
	size_t memory_free_max = 0;

	char chart[CHART_SIZE+2] = {'?'};
	chart[CHART_SIZE+1] = 0;

	do {
		printf("Loop\n");
			mem_print_block(node);
		nodes++;
		if( node->free ){
			memory_free_total += node->size;

			if( node->size > memory_free_max )
				memory_free_max = node->size;

		}

		uint32_t start = ((size_t)node-(size_t)mem) / (double)mem_size * CHART_SIZE;
		uint32_t stop  = ((size_t)node-(size_t)mem + node->size + HEADSIZE)    / (double)mem_size * (CHART_SIZE);


		for( size_t i = start; i < stop; i++ ){
			chart[i] = (node->free) ? '-' : '|';
		}

	} while( (node=node->prev) );


	logf_info("[%s]", chart);
	logf_info("Total free memory: %li MB", memory_free_total/1024/1024);
	logf_info("Largest single block: %li MB", memory_free_max/1024/1024);
	logf_info("Total nodes: %i", nodes);
	logf_info("Fragmentation: %i %%", (int)
		((memory_free_total - memory_free_max) / (double)memory_free_total * 100)
	);

}

int mem_init(size_t heap_size){

	shell_bind_command("memory_list_labels", &command_memory_labels);
	shell_bind_command("memory_status", &command_memory_status);

	mem_size = heap_size;
	mem = malloc( mem_size );

	struct MemoryNode *head = mem;
	head->prev = NULL;
	head->next = NULL;
	head->size = heap_size - HEADSIZE - HEADSIZE;
	head->free = 1;


	struct MemoryNode *tail = (void*)mem + mem_size - HEADSIZE;
	tail->prev = head;
	tail->next = NULL;
	tail->size = 0;
	tail->free = 0;

	mem_tail = tail;


	head->next = tail;


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

	//double start = ctx_time();

	#ifdef USE_SYSTEM_ALLOC
		void* p = malloc( bytes );
	#else
		void* p = mem_block_alloc(bytes);
	#endif
	if( p == NULL  ) {
		command_memory_status(0, NULL);
		//mem_block_print_debug();
		logf_error( "Allocation failure! %lu b %s", bytes, label  );
		panic();
	}

	//double stop = ctx_time();
	//printf("alloc in %f ms %i K\n", (stop-start)*1000, bytes / 1024);

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
	//void* p = calloc( bytes, 1 );
	

	#ifdef USE_SYSTEM_ALLOC
		void* p = malloc( bytes );
	#else
		void* p = mem_block_alloc(bytes);
	#endif
	if( p == NULL  ) {
		logf_error( "Allocation failure! %lu b %s", bytes, label  );
		panic();
	}
	memset( p, 0, bytes );

	alloc_table[at_index].p = p;
	alloc_table[at_index].size = bytes;
	alloc_table[at_index].label = label;
	at_index++;
	total_bytes += bytes;

	pthread_mutex_unlock( &mem_mtx );

	return p;
}

void *_mem_free( void* p, char*label){
	
	pthread_mutex_lock( &mem_mtx );

	if( p == NULL  ){
		logf_error(
			"Deallocation error:\n" 
			"Tried to deallocate a NULL pointer. Source:\n%s",
			
			label
		);
		panic();
		return NULL;
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
		return NULL;
	}
	// Decrement index and move the last item to current place
		

	//double start = ctx_time();

	#ifdef USE_SYSTEM_ALLOC
		free( p );
	#else
		mem_block_free(p);
	#endif

	//double stop = ctx_time();
	//printf("free in %f ms\n", (stop-start)*1000);

//	logf_info("Freed %lu", alloc_table[_at_i].size);

	total_bytes -= alloc_table[_at_i].size;

	at_index--;	
	alloc_table[_at_i].p 	= alloc_table[at_index].p;
	alloc_table[_at_i].size = alloc_table[at_index].size;

	pthread_mutex_unlock( &mem_mtx );

	return NULL;
}
