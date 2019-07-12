#include "res.h"
#include <string.h>

#include "mem.h"

// Stores and provides resourse files
// Linked with files.o which is built with ld and contains binary blobs.
// More in the header file


LS(DEF)

unsigned const char* res_file(File file){
	switch (file){
		LS(CASE_START)
	}
	return NULL;
}

size_t res_size(File file){
	switch (file){
		LS(CASE_SIZE)
	}
	return 0;
}

// Allocates a null terminated string. UNALLOC YOURSELF!
char* res_strcpy(File file){
		
	char * str = mem_alloc( res_size(file)+1 );
	
	memcpy( str, res_file(file), res_size(file) );
	str[ res_size(file) ] = '\0';
	
	return str;
}
