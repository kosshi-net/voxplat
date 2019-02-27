#ifndef	GFX_GFX_H
#define	GFX_GFX_H 1

#include <glad/glad.h>

int gfx_init(void);

GLuint gfx_create_shader(const char*vertSrc, const char*fragSrc);

#endif
