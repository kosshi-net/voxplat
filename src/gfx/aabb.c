#define __FILENAME__ "gfx/aabb.c"
#include "gfx/aabb.h"

#include "gfx.h"
#include "gfx/camera.h"
#include "res.h"
#include "mem.h"

#include <stdlib.h>
#include <glad/glad.h>

GLuint aabb_vao;
GLuint aabb_shader;
GLuint aabb_shader_a_coord;
GLuint aabb_shader_u_color;

GLuint aabb_shader_u_far;

GLuint aabb_shader_u_projection;
GLuint aabb_shader_u_view_rotation;
GLuint aabb_shader_u_view_translation;
GLuint aabb_vbo;

float *aabb_local_geom;
uint32_t aabb_local_geom_len = 0;

int gfx_aabb_init(void)
{
	aabb_local_geom = mem_alloc( 20*1024*1024 );

	glGenVertexArrays(1, &aabb_vao);
	glBindVertexArray(aabb_vao);

	char *vert_src = res_strcpy( wire_vert_glsl  );
	char *frag_src = res_strcpy( wire_frag_glsl  );
	aabb_shader = gfx_create_shader( vert_src, frag_src );
	mem_free( vert_src );	
	mem_free( frag_src );

	glUseProgram(aabb_shader);
	aabb_shader_a_coord = glGetAttribLocation (aabb_shader, "aVertex");
	aabb_shader_u_color = glGetUniformLocation(aabb_shader, "uColor");


	aabb_shader_u_projection =	glGetUniformLocation(aabb_shader, 
				 "u_projection");

	aabb_shader_u_view_rotation =	glGetUniformLocation(aabb_shader, 
				 "u_view_rotation");

	aabb_shader_u_view_translation =	glGetUniformLocation(aabb_shader, 
				 "u_view_translation");

	aabb_shader_u_far =	glGetUniformLocation(aabb_shader, 
			   "u_far");


	glGenBuffers( 1, &aabb_vbo );
	glEnableVertexAttribArray( aabb_shader_a_coord );
	glBindBuffer( GL_ARRAY_BUFFER, aabb_vbo );

	glVertexAttribPointer( aabb_shader_a_coord, 3, GL_FLOAT, GL_FALSE, 0, NULL );

	return 0;
}

void gfx_aabb_push( float *min, float *max )
{
// min and max are float[3] in -1 to +1 range
	float _temp[] = {
		// BOTTOM
		-min[0], -min[1], -min[2],
		-min[0], -min[1], -max[2],
		-min[0], -min[1], -max[2],
		-max[0], -min[1], -max[2],
		-max[0], -min[1], -max[2],
		-max[0], -min[1], -min[2],
		-max[0], -min[1], -min[2],
		-min[0], -min[1], -min[2],

		// TOP
		-min[0], -max[1], -min[2],
		-min[0], -max[1], -max[2],
		-min[0], -max[1], -max[2],
		-max[0], -max[1], -max[2],
		-max[0], -max[1], -max[2],
		-max[0], -max[1], -min[2],
		-max[0], -max[1], -min[2],
		-min[0], -max[1], -min[2],

		// CONNECT
		-min[0], -min[1], -min[2],
		-min[0], -max[1], -min[2],
		-min[0], -min[1], -max[2],
		-min[0], -max[1], -max[2],
		-max[0], -min[1], -min[2],
		-max[0], -max[1], -min[2],
		-max[0], -min[1], -max[2],
		-max[0], -max[1], -max[2],
	};
	for( int i = 0; i < sizeof(_temp)/sizeof(float); i++ ){
		aabb_local_geom[aabb_local_geom_len++] = _temp[i];
	}

}

void gfx_aabb_draw( struct Camera *cam, float *color )
{	


	glUseProgram(aabb_shader);
	glBindVertexArray(aabb_vao);

	glEnable( GL_DEPTH_TEST ); // ???
	//glDisable( GL_CULL_FACE );

	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer( GL_ARRAY_BUFFER, aabb_vbo );
	glBufferData(
		GL_ARRAY_BUFFER, 
		aabb_local_geom_len*sizeof(float), 
		aabb_local_geom, 
		GL_STATIC_DRAW
	);


	glUniform4fv( aabb_shader_u_color, 1, color );

	glUniformMatrix4fv(
		aabb_shader_u_projection, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_projection
	);

	glUniformMatrix4fv(
		aabb_shader_u_view_rotation, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_rotation
	);


	glUniform1f(
		aabb_shader_u_far,
		cam->far
	);

	glUniformMatrix4fv(
		aabb_shader_u_view_translation, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_translation
	);

	glDrawArrays(GL_LINES, 0, aabb_local_geom_len/3);

	aabb_local_geom_len = 0;

	return;
};
	