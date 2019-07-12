#define __FILENAME__ "gfx/vsplat.c"
#include "gfx/vsplat.h"

#include "gfx.h"
#include "gfx/fcull.h"

#include "res.h"
#include "mem.h"
#include "ctx.h"
#include "chunkset.h"
#include "../shell.h"
#include "event.h"


#include <stdlib.h>
#include <glad/glad.h>

#include <cglm/cglm.h>
#include <stb_image.h>

GLuint vsplat_shader;
GLuint vsplat_shader_aVertex;
GLuint vsplat_shader_aColor;

GLuint vsplat_shader_u_projection;
GLuint vsplat_shader_u_inv_proj_view_rot;
GLuint vsplat_shader_u_view_rotation;
GLuint vsplat_shader_u_view_translation;
GLuint vsplat_shader_uViewportSize;
GLuint vsplat_shader_u_far;

GLuint vsplat_shader_u_lod;

GLuint vsplat_texture;
GLuint vsplat_shader_uTex;
GLuint vsplat_shader_tex;

GLuint vsplat_vao;


int enable_stencil = 0;
void command_toggle_stencil( int argc, char **argv ){
	enable_stencil = !enable_stencil;
	logf_info("%i", enable_stencil);
}


int gfx_vsplat_init(void)
{
	shell_bind_command("toggle_stencil",	&command_toggle_stencil);

	char* vert_src = res_strcpy( splat_vert_glsl  );
	char* frag_src = res_strcpy( splat_frag_glsl  );
	vsplat_shader = gfx_create_shader( vert_src, frag_src );
	mem_free( vert_src );
	mem_free( frag_src );


	//glGenVertexArrays(1, &vsplat_vao);
	//glBindVertexArray(vsplat_vao);

	glUseProgram(vsplat_shader);

	vsplat_shader_u_projection =	glGetUniformLocation(vsplat_shader, 
				 "u_projection");

	vsplat_shader_u_inv_proj_view_rot =	glGetUniformLocation(vsplat_shader, 
				 "u_inv_proj_view_rot");

	vsplat_shader_u_view_rotation =	glGetUniformLocation(vsplat_shader, 
				 "u_view_rotation");

	vsplat_shader_u_view_translation =	glGetUniformLocation(vsplat_shader, 
				 "u_view_translation");

	vsplat_shader_uViewportSize = glGetUniformLocation(vsplat_shader, 
				 "uViewport");

	vsplat_shader_uTex = glGetUniformLocation(vsplat_shader, 
				 "uTex");

	vsplat_shader_u_far = glGetUniformLocation(vsplat_shader, 
				 "u_far");

	vsplat_shader_u_lod = glGetUniformLocation(vsplat_shader, 
				 "u_lod");

	vsplat_shader_aVertex =
		glGetAttribLocation	(vsplat_shader, "aVertex");

	vsplat_shader_aColor =
		glGetAttribLocation	(vsplat_shader, "aColor");



    int x,y,n;
    unsigned char *data = stbi_load_from_memory(
    	res_file(noise_png), res_size(noise_png)
    	, &x, &y, &n, 0
    );

	glGenTextures(1, &vsplat_texture);
	glBindTexture(GL_TEXTURE_2D, vsplat_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);


	return 0;
}

void gfx_vsplat_draw( 
	struct Camera *cam,
	struct ChunkSet *set,
	struct ChunkMD **queue,
	uint32_t queue_count,
	uint32_t *item_count
){	

	int width, height;
	ctx_get_window_size( &width, &height  );

	glUseProgram(vsplat_shader);
	//glBindVertexArray(vsplat_vao);
	

	if(enable_stencil)	
	glEnable(GL_STENCIL_TEST);
	glStencilMask(0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

	glEnable( GL_PROGRAM_POINT_SIZE  );



	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable( GL_CULL_FACE );


	glUniformMatrix4fv(
		vsplat_shader_u_projection, 
		1, GL_FALSE, 
		(const GLfloat*) cam->projection
	);

	glUniformMatrix4fv(
		vsplat_shader_u_inv_proj_view_rot, 
		1, GL_FALSE, 
		(const GLfloat*) cam->inv_proj_view_rot
	);

	glUniformMatrix4fv(
		vsplat_shader_u_view_translation, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_translation
	);

	glUniformMatrix4fv(
		vsplat_shader_u_view_rotation, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_rotation
	);

	glUniform2f(
		vsplat_shader_uViewportSize, 
		(float)width, (float)height
	);

	glUniform1f(
		vsplat_shader_u_far,
		cam->far_plane
	);

	glActiveTexture(	GL_TEXTURE0 );
	glBindTexture(		GL_TEXTURE_2D, vsplat_texture );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );


	//uint32_t item_count = 0;


	uint32_t upload = 10;

	for( int i = 0; i < queue_count; i++  ){
		struct ChunkMD *c = queue[i];

		if( c->gl_vbo == 0 ) goto skip_draw;
		if(c->gl_vbo_lod == 0) goto skip_draw; 

		// RENDER
		glBindVertexArray(c->gl_vao);

		int lod = c->lod;
		if(!lod) lod = 1;

		glUniform1f(
			vsplat_shader_u_lod,
			1<<(lod-1)
		);

/*
		uint32_t start = c->gl_vbo_segments[lod-1];
		if( lod == 3 )
			//continue;
			start = c->gl_vbo_segments[1]+c->gl_vbo_segments[2];
*/
		uint32_t start = 0;
		for (int i = 1; i < lod; ++i)
		{
			start += c->gl_vbo_segments[i];
		}

		glDrawArrays( GL_POINTS, 
			start/4, 
			c->gl_vbo_segments[lod]  /4
		);

		*item_count += c->gl_vbo_segments[c->lod]  / 4;


		skip_draw:
		if(c->gl_vbo_local_items == 0 && c->gl_vbo ){
			glDeleteBuffers( 1, &c->gl_vbo );
			c->gl_vbo = 0;
			//logf_info("Deleted buffer");

			continue;
		}

		glBindVertexArray(0);
		if( c->gl_vbo_local != NULL ){

		
			if( c->gl_vbo == 0  ) {
				if( !c->gl_vao ){
					glGenVertexArrays(1, &c->gl_vao);
					glBindVertexArray(c->gl_vao);
				}

				glEnableVertexAttribArray(vsplat_shader_aVertex);
				glEnableVertexAttribArray(vsplat_shader_aColor);

				glGenBuffers(1, &c->gl_vbo );
			}

			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
			glBindBuffer( GL_ARRAY_BUFFER, 0 );

			glBindVertexArray(c->gl_vao);
			
			glBindBuffer( GL_ARRAY_BUFFER, c->gl_vbo );

			glVertexAttribPointer(
				vsplat_shader_aVertex, 3, GL_SHORT, GL_FALSE, 
				4*sizeof(int16_t),
				NULL
			);
			
			glVertexAttribIPointer(
				vsplat_shader_aColor, 1, GL_SHORT,
				4*sizeof(int16_t), 
				(const void*) (3*sizeof(int16_t))
			);

			glBufferData(
				GL_ARRAY_BUFFER, c->gl_vbo_local_items *sizeof(int16_t),
				c->gl_vbo_local, GL_STATIC_DRAW
			);

			glBindVertexArray(0);

			//mem_free(c->gl_vbo_local);
			c->gl_vbo_items = c->gl_vbo_local_items;
			c->gl_vbo_lod = c->gl_vbo_local_lod;
			c->gl_vbo_local = NULL;

			for (int i = 0; i < 8; ++i)
			{
				c->gl_vbo_segments[i] = c->gl_vbo_local_segments[i];
			}


		
			if( c->gl_ibo_local != NULL ){
			
				if( c->gl_ibo == 0  ) glGenBuffers(1, &c->gl_ibo );
				
				glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, c->gl_ibo );

				glBufferData(
					GL_ELEMENT_ARRAY_BUFFER, c->gl_ibo_local_items *sizeof(int32_t),
					c->gl_ibo_local, GL_STATIC_DRAW
				);

				//mem_free(c->gl_ibo_local);
				c->gl_ibo_items = c->gl_ibo_local_items;
				c->gl_ibo_local = NULL;
				
			}
			if(c->mesher_lock) 
				*c->mesher_lock = 0;
			
		}


	}

	glBindVertexArray(0);
	
	glDisable(GL_STENCIL_TEST);

	return;
};
	