#define __FILENAME__ "gfx/sky.c"
#include "gfx/sky.h"

#include "gfx.h"
#include "gfx/camera.h"
#include "res.h"
#include "mem.h"
#include "ctx.h"

#include <stdlib.h>
#include <glad/glad.h>

GLuint sky_vao;
GLuint sky_shader;
GLuint sky_shader_a_coord;
GLuint sky_shader_u_color;
GLuint sky_shader_u_inv_proj_view_rot;
GLuint sky_shader_uViewport;
GLuint sky_vbo;


int gfx_sky_init(void)
{
	glGenVertexArrays(1, &sky_vao);
	glBindVertexArray(sky_vao);

	char *vert_src = res_strcpy( sky_vert_glsl  );
	char *frag_src = res_strcpy( sky_frag_glsl  );
	sky_shader = gfx_create_shader( vert_src, frag_src );
	mem_free( vert_src );	
	mem_free( frag_src );

	glUseProgram(sky_shader);
	sky_shader_a_coord = glGetAttribLocation (sky_shader, 
			  "aCoord");
	sky_shader_u_inv_proj_view_rot =	glGetUniformLocation(sky_shader, 
			  "u_inv_proj_view_rot");
	sky_shader_uViewport =	glGetUniformLocation(sky_shader, 
			  "uViewport");


	glGenBuffers( 1, &sky_vbo );
	glEnableVertexAttribArray( sky_shader_a_coord );
	glBindBuffer( GL_ARRAY_BUFFER, sky_vbo );

	glVertexAttribPointer( sky_shader_a_coord, 2, GL_FLOAT, GL_FALSE, 0, NULL );

	return 0;
}

void gfx_sky_draw(struct Camera *cam)
{	// min and max are float[2] in -1 to +1 range
	float min[2] = {-1.0,-1.0};
	float max[2] = { 1.0, 1.0};
	float sky_local_geom[] = {
		min[0], min[1],
		min[0], max[1],
		max[0], min[1],
		max[0], max[1],
	};

	int width, height;
	ctx_get_window_size( &width, &height  );

	glUseProgram(sky_shader);
	glBindVertexArray(sky_vao);

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer( GL_ARRAY_BUFFER, sky_vbo );
	glBufferData(
		GL_ARRAY_BUFFER, 
		sizeof(sky_local_geom), 
		sky_local_geom, 
		GL_STATIC_DRAW
	);

	glUniform2f(
		sky_shader_uViewport, 
		(float)width, (float)height
	);
	glUniformMatrix4fv(
		sky_shader_u_inv_proj_view_rot, 
		1, GL_FALSE, 
		(const GLfloat*) cam->inv_proj_view_rot
	);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	return;
};
	