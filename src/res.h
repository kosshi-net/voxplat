#ifndef RES_RES_H
#define RES_RES_H 1

#include <config.h>
#include <stdlib.h>

// List resource files here!
// Replace dots and other wierd characters with _
// These will also be defined as ENUMs in C

#define LS( add )\
	add(test_txt)\
	add(text_vert_glsl)\
	add(text_frag_glsl)\
	add(splat_frag_glsl)\
	add(splat_vert_glsl)\
	add(mesh_vert_glsl)\
	add(mesh_frag_glsl)\
	add(rect_frag_glsl)\
	add(rect_vert_glsl)\
	add(wire_frag_glsl)\
	add(wire_vert_glsl)\
	add(map_vxl)\
	add(font_ttf)\
	add(seven_png)\
	add(noise_png)\

// Debug tip:
// If you get a bunch of undefined references to [_]binary*,
// "objdump -t resources.o"
// Do note that MinGW seems to strip the first underscore.

#if __MINGW32__
	#define START(f) 	binary_ ## f ## _start
	#define END(f) 		binary_ ## f ## _end
#else
	#define START(f) 	_binary_ ## f ## _start
	#define END(f) 		_binary_ ## f ## _end
#endif

#define SIZE(f) 	END(f) - START(f)
#define CASE_START(f) 	case f: return START(f);
#define CASE_SIZE(f) 	case f: return SIZE(f);
#define ENUM(f) f,
#define DEF(f) \
	extern const unsigned char START(f)[]; \
	extern const unsigned char END(f)[];


typedef enum {
	LS(ENUM)
} File;

// Returns a pointer to start of the file
const unsigned char* 	res_file(File);
size_t					res_size(File);

// malloc()s a properly sized null-terminated string from the blob
// Remember to free() it yourself!
char*	res_strcpy(File);

// Example usage
// char* str = res_strcpy(test_txt);
// print( "test.txt: %c", str );
// free(str);


#endif
