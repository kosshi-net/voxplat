#ifndef cfg_H
#define cfg_H 1

#include <stdint.h>
#include <stdlib.h>

// char = bool

struct Config
{
	
	uint8_t 	chunk_size;
	uint8_t 	world_size[3];

	char 		opengl_compat;
	char 		opengl_debug;

	uint8_t 	opengl_minor;
	uint8_t 	opengl_major;

	size_t 		heap;

	char 		rle;
	double 		rle_timeout;
	double 		rle_at_start;

};

void cfg_init(int argc, char** argv);

struct Config *cfg_get();

#endif
