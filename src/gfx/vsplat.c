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


int gfx_vsplat_init(void)
{
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


void gfx_svl_draw( 
	struct Camera *cam,
	struct ChunkSet *set,
	struct GeometrySVL **queue,
	uint32_t queue_count,
	uint32_t *item_count
){	

	int width, height;
	ctx_get_window_size( &width, &height  );

	glUseProgram(vsplat_shader);

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



	for( int i = 0; i < queue_count; i++  ){
		struct GeometrySVL *g = queue[i];

		if( g->vbo == 0 ) continue;

		// RENDER
		glBindVertexArray(g->vao);

		uint8_t lod = g->lod;

		glUniform1f(
			vsplat_shader_u_lod,
			1<<( lod )
		);

		glDrawArrays( GL_POINTS, 
			0, 
			g->vbo_items/4
		);

		*item_count += g->vbo_items/4;

	}
}

#define MIN(a,b) (((a)<(b))?(a):(b))

void gfx_update_svl( 
	struct ChunkSet *set,
	uint32_t index
){	

	struct ChunkMD *c = &set->chunks[ index ];

	glUseProgram(vsplat_shader);

	glBindVertexArray(0);


	for (int lod = 0; lod < MAX_LOD_LEVEL; ++lod)
	{
		uint16_t octree[3];
		uint8_t octree_bitw[3];
		uint16_t cs_min[3];
		uint16_t cs_max[3];
		for (int i = 0; i < 3; ++i) {
			octree_bitw[i] = set->max_bitw[i] - (MIN( lod, set->max_bitw[i] )); // this fucks up
			octree[i] = c->offset[i] >> lod;

			cs_min[i] = (octree[i]  )	<< lod;
			cs_max[i] = (octree[i]+1)	<< lod;
			if( cs_max[i] > set->max[i] ) cs_max[i] = set->max[i];
		}
		
		//if( cs_max[1] > set->max[1] ) cs_max[1] = set->max[1];



		//uint32_t octree_index = flatten3( octree, octree_bitw );
		uint32_t octree_index = flatten3( octree, octree_bitw );

		struct GeometrySVL *g = &set->gsvl [ lod ][ octree_index ];

		g->lod = lod;
	
		if( g->vbo == 0  ) {
			glGenVertexArrays(1,&g->vao);
			glBindVertexArray(   g->vao);
			glEnableVertexAttribArray(vsplat_shader_aVertex);
			glEnableVertexAttribArray(vsplat_shader_aColor);
			glGenBuffers(1, &g->vbo );
		}

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		glBindBuffer( GL_ARRAY_BUFFER, 0 );

		glBindVertexArray(g->vao);
		
		glBindBuffer( GL_ARRAY_BUFFER, g->vbo );

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

		size_t reserve_size = 0;

		uint16_t scan[3];
		for (scan[0] = cs_min[0]; scan[0] < cs_max[0]; ++scan[0])
		for (scan[1] = cs_min[1]; scan[1] < cs_max[1]; ++scan[1])
		for (scan[2] = cs_min[2]; scan[2] < cs_max[2]; ++scan[2])
		{
			struct ChunkMD *sub = &set->chunks[flatten3( scan, set->max_bitw )];

			pthread_mutex_lock( &sub->mutex_svl );

			uint32_t upload_items = sub->svl_items[lod];

			if(!upload_items) continue;

			reserve_size += upload_items;
		}

		// Allocate
		if(reserve_size)
		glBufferData(
			GL_ARRAY_BUFFER, reserve_size*sizeof(uint16_t), NULL, GL_STATIC_DRAW
		);

		size_t total_items = 0;

		for (scan[0] = cs_min[0]; scan[0] < cs_max[0]; ++scan[0])
		for (scan[1] = cs_min[1]; scan[1] < cs_max[1]; ++scan[1])
		for (scan[2] = cs_min[2]; scan[2] < cs_max[2]; ++scan[2])
		{
			struct ChunkMD *sub = &set->chunks[flatten3( scan, set->max_bitw )];

			uint32_t upload_items = sub->svl_items[lod];

			if(!upload_items) goto unlock;

			uint32_t upload_start = 0;
			for (int i = 0; i < lod; ++i) {
				upload_start += sub->svl_items[i];
			}

			if( total_items + upload_items > reserve_size ) panic();

			glBufferSubData(
				GL_ARRAY_BUFFER, 
				// Offset
				total_items  * sizeof(int16_t),
				// Size
				upload_items * sizeof(int16_t), 
				// Data
				&((uint16_t*)sub->svl)[ upload_start ] 
			);

			total_items += upload_items;

			unlock:
			pthread_mutex_unlock( &sub->mutex_svl );
			;
		}

		g->vbo_items = reserve_size;

		if( reserve_size == 0 ){
			glDeleteVertexArrays(1, &g->vao);
			glDeleteBuffers(1, 		&g->vbo);
			g->vao = 0;
			g->vbo = 0;
		}

		glBindVertexArray(0);

	}

	c->svl_dirty = 0;

	glBindVertexArray(0);
	return;
};
