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


	float 		sensitivity;
	float 		speed;

	char 		debug_show_chunk_borders;
	char 		debug_disable_lod;

	uint32_t 	draw_distance;

	uint32_t 	brush_color;
	char 		brush_color_random;
	uint32_t 	brush_size;


};

void cfg_init(int argc, char** argv);

struct Config *cfg_get();

#endif
