#ifndef MEM_H
#define MEM_H 1

#include <stdlib.h>

#ifndef __FILENAME__
#define __FILENAME__ __FILE__ 
#endif
#define S1(x) #x
#define S2(x) S1(x)
#define SOURCE __FILENAME__ "::" S2(__LINE__)

#define mem_alloc(s) _mem_alloc(s, SOURCE )
#define mem_calloc(s) _mem_calloc(s, SOURCE )
#define mem_free(s) _mem_free(s, SOURCE )

int mem_init(size_t);

void  *_mem_alloc  	( size_t, char*label );
void  *_mem_calloc	( size_t, char*label );
void  *_mem_free	( void* , char*label );
// free always returns null

void mem_log_debug(void);

size_t mem_dynamic_allocation();

#endif
