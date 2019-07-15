#define __FILENAME__ "cfg.c"
#include "cfg.h"

#include <config.h>

#include "shell.h"
#include <string.h>
#include <stdio.h>

#define clz(x) __builtin_clz(x)
#define ctz(x) __builtin_ctz(x)

struct Config config;



enum CLIOptions {
	option_default,
	option_help,
	option_version,
	option_opengl_compat,
	option_opengl_debug,
	option_chunk_size,
	option_world_size,
	option_heap
};

enum CLIOptions parse_cli_option( char* str ){
	if( strcmp( str, "--help"
	)==0) return option_help;

	if( strcmp( str, "--version"
	)==0) return option_version;

	if( strcmp( str, "--opengl_compat"
	)==0) return option_opengl_compat;

	if( strcmp( str, "--opengl_debug"
	)==0) return option_opengl_debug;

	if( strcmp( str, "--world_size"
	)==0) return option_world_size;

	if( strcmp( str, "--chunk_size"
	)==0) return option_chunk_size;

	if( strcmp( str, "--heap"
	)==0) return option_heap;

	return option_default;
}


void cfg_init( int argc, char **argv ){

	// Set defaults and stuff
	config.chunk_size 		= 6;
	config.heap 			= parse_long( "512M" );
	config.sensitivity		= 0.15;
	config.speed			= 75.0;
	config.draw_distance	= 1024*4;
	config.brush_color		= 63;
	config.brush_size		= 4;

	uint32_t default_chunk_size = 64;
	uint32_t default_world_size[] = { 2048, 256, 2048 };

	for (int i = 1; i < argc; ++i)
	{
		char *arg = argv[i];


		switch( parse_cli_option( arg ) ){
			case option_help:
				printf("--help\n");
				printf("--version\n");
				printf("--opengl_compat\n");
				printf("--opengl_debug\n");
				printf("--world_size <int>\n");
				printf("--chunk_size <int> <int> <int>\n");
				printf("--heap <int>\n");
				exit(0);
				break;

			case option_version:
				printf( "%s %s.%s\n", PROJECT_NAME,VERSION_MAJOR,VERSION_MINOR);
				exit(0);
				break;

			case option_opengl_compat:
				config.opengl_compat = 1;
				break;

			case option_opengl_debug:
				config.opengl_debug = 1;
				break;

			case option_chunk_size:
				if( i+1 >= argc ){
					printf("Not enough arguments\n");
					exit(1);
				}
				default_chunk_size = parse_long(argv[++i]);
				break;

			case option_world_size:
				if( i+3 >= argc ){ 
					printf("Not enough arguments\n");
					exit(1);
				}
				default_world_size[0] = parse_long(argv[++i]);
				default_world_size[1] = parse_long(argv[++i]);
				default_world_size[2] = parse_long(argv[++i]);
				break;

			case option_heap:
				i++;
				if( i >= argc ){ 
					printf("Usage: --heap <number followed by K, M or G>\nExample: --heap 512M\n");
					exit(1);
				}
				config.heap = parse_long( argv[i] );
				if( config.heap == 0 ) {
					printf("Bad heap size\n");
					exit(1);
				}
				break;

			default:
				printf("Bad option %s\n", arg);
				printf("See --help\n");
				exit(1);
				break;
		}
	}


	config.chunk_size = ctz( default_chunk_size );

	uint32_t x = default_world_size[0] / (1<<config.chunk_size);
	uint32_t y = default_world_size[1] / (1<<config.chunk_size);
	uint32_t z = default_world_size[2] / (1<<config.chunk_size);
	config.world_size[0] = ctz(x);
	config.world_size[1] = ctz(y);
	config.world_size[2] = ctz(z);


}


struct Config *cfg_get(){
	return &config;
}

