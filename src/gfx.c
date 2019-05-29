#define __FILENAME__ "gfx.c"

#include "gfx.h"
#include "gfx/shell.h"
#include "gfx/text.h"
#include "gfx/rect.h"
#include "gfx/aabb.h"
#include "gfx/sky.h"

#include "event.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int gfx_init() {
	log_info( "Init" );

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	if ( gfx_text_init()	) return 1;
	if ( gfx_rect_init()	) return 1;
	if ( gfx_shell_init()	) return 1;
	if ( gfx_aabb_init()	) return 1;
	if ( gfx_sky_init()	) return 1;

	log_info( "Init ok" );
	return 0;
}

GLuint gfx_create_shader(const char* vert_src, const char* frag_src){
		
	//log_info( "Compiling shader..." );

	int InfoLogLength = 0;

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vert_src, NULL);
	glCompileShader(vs);

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &frag_src, NULL);
	glCompileShader(fs);

	glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &InfoLogLength);
	
	if ( InfoLogLength > 0 ){
		char * err = (char*)malloc(InfoLogLength+1);
		glGetShaderInfoLog(vs, InfoLogLength, NULL, 
			&err[0]);
		logf_error("Vertex shader failed to compile!\n%s", 
			&err[0]);
	}

	glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if ( InfoLogLength > 0 ){
		char * err = (char*)malloc(InfoLogLength+1);
		glGetShaderInfoLog(fs, InfoLogLength, NULL, 
			&err[0]);
		logf_error("Fragment shader failed to compile!\n%s", 
			&err[0]);
	}

	GLuint shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);

	glGetProgramiv(shader_program, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		char * err = (char*)malloc(InfoLogLength+1);
		glGetProgramInfoLog(shader_program, InfoLogLength, NULL, &err[0]);
		logf_error("Shaderprogram link failure!\n%s", &err[0]);
	}
	
	if ( InfoLogLength==0  ){
		//log_info( "Shader compile ok" );
	}else{
		panic();
	}

	return shader_program;
}
