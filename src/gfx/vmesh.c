#define __FILENAME__ "gfx/vmesh.c"
#include "gfx/vmesh.h"

#include "gfx.h"
#include "gfx/fcull.h"

#include "res.h"
#include "mem.h"
#include "ctx.h"
#include "chunkset.h"


#include <stdlib.h>
#include <glad/glad.h>

#include <cglm/cglm.h>
#include <stb_image.h>

GLuint vmesh_shader;
GLuint vmesh_shader_aVertex;
GLuint vmesh_shader_aColor;

GLuint vmesh_shader_u_projection;
GLuint vmesh_shader_u_inv_proj_view_rot;
GLuint vmesh_shader_u_view_rotation;
GLuint vmesh_shader_u_view_translation;
GLuint vmesh_shader_u_far;

GLuint vmesh_shader_uViewportSize;

GLuint vmesh_texture;
GLuint vmesh_shader_uTex;
GLuint vmesh_shader_tex;

GLuint vmesh_vao;


int gfx_vmesh_init(void)
{
	char* vert_src = res_strcpy( mesh_vert_glsl  );
	char* frag_src = res_strcpy( mesh_frag_glsl  );
	vmesh_shader = gfx_create_shader( vert_src, frag_src );
	mem_free( vert_src );
	mem_free( frag_src );


	glGenVertexArrays(1, &vmesh_vao);
	glBindVertexArray(vmesh_vao);

	glUseProgram(vmesh_shader);

	vmesh_shader_u_projection =	glGetUniformLocation(vmesh_shader, 
				 "u_projection");

	vmesh_shader_u_inv_proj_view_rot =	glGetUniformLocation(vmesh_shader, 
				 "u_inv_proj_view_rot");

	vmesh_shader_u_view_rotation =	glGetUniformLocation(vmesh_shader, 
				 "u_view_rotation");

	vmesh_shader_u_view_translation =	glGetUniformLocation(vmesh_shader, 
				 "u_view_translation");

	vmesh_shader_uViewportSize = glGetUniformLocation(vmesh_shader, 
				"uViewport");

	vmesh_shader_u_far = glGetUniformLocation(vmesh_shader, 
				"u_far");

	vmesh_shader_uTex = glGetUniformLocation(vmesh_shader, 
				"uTex");

	vmesh_shader_aVertex =
		glGetAttribLocation	(vmesh_shader, "aVertex");
	glEnableVertexAttribArray(vmesh_shader_aVertex);

	vmesh_shader_aColor =
		glGetAttribLocation	(vmesh_shader, "aColor");
	glEnableVertexAttribArray(vmesh_shader_aColor);



    int x,y,n;
    unsigned char *data = stbi_load_from_memory(
    	res_file(noise_png), res_size(noise_png)
    	, &x, &y, &n, 0
    );

	glGenTextures(1, &vmesh_texture);
	glBindTexture(GL_TEXTURE_2D, vmesh_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);


	return 0;
}

void gfx_vmesh_draw( 
	struct Camera *cam,
	struct ChunkSet *set,
	struct GeometryMesh **queue,
	uint32_t queue_count,
	uint32_t *item_count
){	

	int width, height;
	ctx_get_window_size( &width, &height  );

	glUseProgram(vmesh_shader);
	glBindVertexArray(vmesh_vao);

	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );

	glUniformMatrix4fv(
		vmesh_shader_u_projection, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_projection
	);

	glUniformMatrix4fv(
		vmesh_shader_u_view_translation, 
		1, GL_FALSE, 
		(const GLfloat*) cam->view_translation
	);

	glUniform1f(
		vmesh_shader_u_far,
		cam->far_plane
	);

	glUniform2f(
		vmesh_shader_uViewportSize, 
		(float)width, (float)height
	);

	glActiveTexture(	GL_TEXTURE0 );
	glBindTexture(		GL_TEXTURE_2D, vmesh_texture );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );


	//uint32_t item_count = 0;

	for( int i = 0; i < queue_count; i++  ){
		struct GeometryMesh *g = queue[i];
	
		if( g->vbo == 0 ) continue;

		glBindVertexArray(g->vao);
	
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 	g->ibo);
		glBindBuffer(GL_ARRAY_BUFFER, 			g->vbo);
		
 		glDrawElements(
 		    GL_TRIANGLES,
 		    g->ibo_items,
 		    GL_UNSIGNED_INT,
 		    0
 		);
		*item_count += g->ibo_items;
	}
	glBindVertexArray(0);
	
	glDisable( GL_CULL_FACE );

	return;
};



void gfx_update_mesh( 
	struct ChunkSet *set,
	uint32_t index
){

	glBindVertexArray(0);

	struct ChunkMD *c = &set->chunks[index];
	struct GeometryMesh *g = &set->gmesh[index];

	if( g->vbo == 0  ) {
		glGenVertexArrays(1, &g->vao);
		glBindVertexArray(	  g->vao);

		glEnableVertexAttribArray(vmesh_shader_aVertex);
		glEnableVertexAttribArray(vmesh_shader_aColor);

		glGenBuffers(1, &g->vbo );
		glGenBuffers(1, &g->ibo );
	}

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 	0 );
	glBindBuffer( GL_ARRAY_BUFFER, 			0 );

	glBindVertexArray(g->vao);
	
	glBindBuffer( GL_ARRAY_BUFFER, g->vbo );

	glVertexAttribPointer(
		vmesh_shader_aVertex, 3, GL_SHORT, GL_FALSE, 
		4*sizeof(int16_t),
		NULL
	);
	
	glVertexAttribIPointer(
		vmesh_shader_aColor, 1, GL_SHORT,
		4*sizeof(int16_t), 
		(const void*) (3*sizeof(int16_t))
	);

	glBufferData(
		GL_ARRAY_BUFFER, c->mesh_vbo_items *sizeof(int16_t),
		c->mesh_vbo, GL_STATIC_DRAW
	);

	glBindVertexArray(0);

	g->vbo_items = c->mesh_vbo_items;
	
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, g->ibo );

	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, c->mesh_ibo_items *sizeof(int32_t),
		c->mesh_ibo, GL_STATIC_DRAW
	);

	g->ibo_items = c->mesh_ibo_items;

	c->mesh_dirty = 0;

	glBindVertexArray(0);

	return;
};


void gfx_delete_mesh( 
	struct ChunkSet *set,
	uint32_t index
){

	glBindVertexArray(0);

	struct ChunkMD *c = &set->chunks[index];
	struct GeometryMesh *g = &set->gmesh[index];

	if( g->vbo ) {
		glDeleteBuffers(1, &g->vbo );
		glDeleteBuffers(1, &g->ibo );
		glDeleteVertexArrays(1, &g->vao);

		g->vbo = 0;
		g->ibo = 0;
		g->vao = 0;

	}

	c->mesh_dirty = 0;
	return;
};
