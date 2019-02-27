#define __FILENAME__ "gfx/rect.c"
#include "gfx/rect.h"

#include "gfx.h"
#include "res.h"
#include "mem.h"

#include <stdlib.h>
#include <glad/glad.h>

GLuint rect_vao;
GLuint rect_shader;
GLuint rect_shader_a_coord;
GLuint rect_shader_u_color;
GLuint rect_vbo;


int gfx_rect_init(void)
{
	glGenVertexArrays(1, &rect_vao);
	glBindVertexArray(rect_vao);

	char *vert_src = res_strcpy( rect_vert_glsl  );
	char *frag_src = res_strcpy( rect_frag_glsl  );
	rect_shader = gfx_create_shader( vert_src, frag_src );
	mem_free( vert_src );	
	mem_free( frag_src );

	glUseProgram(rect_shader);
	rect_shader_a_coord = glGetAttribLocation (rect_shader, "aCoord");
	rect_shader_u_color = glGetUniformLocation(rect_shader, "uColor");

	glGenBuffers( 1, &rect_vbo );
	glEnableVertexAttribArray( rect_shader_a_coord );
	glBindBuffer( GL_ARRAY_BUFFER, rect_vbo );

	glVertexAttribPointer( rect_shader_a_coord, 2, GL_FLOAT, GL_FALSE, 0, NULL );

	return 0;
}

void gfx_rect_draw( float *min, float *max, float *color )
{	// min and max are float[2] in -1 to +1 range
	float rect_local_geom[] = {
		min[0], min[1],
		min[0], max[1],
		max[0], min[1],
		max[0], max[1],
	};


	glUseProgram(rect_shader);
	glBindVertexArray(rect_vao);

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer( GL_ARRAY_BUFFER, rect_vbo );
	glBufferData(
		GL_ARRAY_BUFFER, 
		sizeof(rect_local_geom), 
		rect_local_geom, 
		GL_STATIC_DRAW
	);

	glUniform4fv( rect_shader_u_color, 1, color );

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return;
};
	