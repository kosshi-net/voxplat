#define __FILENAME__ "game.c"

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <config.h>

#include "event.h"
#include "ctx.h"
#include "ctx/input.h"

#include "shell.h"

#include "gfx.h"
#include "gfx/crosshair.h"
#include "gfx/text.h"
#include "gfx/shell.h"
#include "gfx/aabb.h"
#include "gfx/fcull.h"

#include "mem.h"
#include "res.h"
#include "chunkset.h"
#include "chunkset/edit.h"
#include "chunkset/gen.h"


#include "cpp/noise.h"

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stb_image.h>

GLuint splat_shader;
GLuint splat_shader_aVertex;
GLuint splat_shader_aColor;
GLuint splat_shader_uPMatrix;
GLuint splat_shader_uPMatrixInverse;
GLuint splat_shader_uViewMat;
GLuint splat_shader_uViewportSize;

GLuint splat_texture;
GLuint splat_shader_uTex;
GLuint splat_shader_tex;

GLuint splat_vao;


double last_time;

float cam_pos[] = {64,256,64};

float cam_yaw 	= 0;
float cam_pitch = 0;


struct ChunkSet *set;


pthread_t mesher_thread;

int mesher_thread_exit = 0;

void mesher_terminate(){
	// Called via event fired in main thread
	mesher_thread_exit = 1;
	logf_info( "Waiting for mesher thread to terminate ..." );
	gfx_shell_exclusive_draw();
	pthread_join( mesher_thread, NULL );
}

void *mesher_loop( void*ptr  ){

	event_bind(EVENT_EXIT, *mesher_terminate);

	while( !mesher_thread_exit ){
		chunkset_manage( set );
		usleep( 5 * 1000  ); // N * 1ms
	}
	pthread_exit(NULL);
	return NULL;
}

const char *renderer;

float sensitivity = 0.05;
void command_sensitivity( int argc, char **argv ){
	if(argc == 2){
		float temp = atof(argv[1]);
		if( temp != 0.0 )
			sensitivity = temp;
	}
	logf_info("%f", sensitivity);
}

float speed = 150;
void command_speed( int argc, char **argv ){
	if(argc == 2){
		float temp = atof(argv[1]);
		if( temp != 0.0 )
			speed = temp;
	}
	logf_info("%f", speed);
}

int show_chunks = 0;
void command_show_chunks( int argc, char **argv ){
	show_chunks = !show_chunks;
	logf_info("%i", show_chunks);
}

void command_chunkset_edit( int argc, char **argv ){
	if( argc == 6 && strcmp(argv[1], "set") == 0 ){
		uint32_t ws[3];
		ws[0] = atoi(argv[2]);
		ws[1] = atoi(argv[3]);
		ws[2] = atoi(argv[4]);
		chunkset_edit_write(
			set,
			ws,
			atoi(argv[5])
		);
	} else {
		logf_info("Usage: chunkset_edit set x y z id");
	}
}

int game_init(){

	shell_bind_command("sensitivity", 	&command_sensitivity);
	shell_bind_command("speed", 		&command_speed);
	shell_bind_command("chunkset_edit", &command_chunkset_edit);
	shell_bind_command("show_chunks", 	&command_show_chunks);


	// Compile shader
	//char* vert_src = res_strcpy( splat_vert_glsl  );
	char* vert_src = res_strcpy( splat_vert_glsl  );
	char* frag_src = res_strcpy( splat_frag_glsl  );
	splat_shader = gfx_create_shader( vert_src, frag_src );
	mem_free( vert_src );
	mem_free( frag_src );


	// Create world and geometry
	// 2 4
	// 3 8
	// 4 16
	// 5 32
	// 6 64
	// 7 128
	set = chunkset_create( 
		//7, (uint8_t[]){4,2,4}
		7, (uint8_t[]){4,2,4}
		//6, (uint8_t[]){5,3,5}
		//5, (uint8_t[]){6,4,6}
	);
	
	chunkset_clear( set );
	chunkset_gen( set );
	


	glGenVertexArrays(1, &splat_vao);
	glBindVertexArray(splat_vao);

	glUseProgram(splat_shader);

	splat_shader_uPMatrix =	glGetUniformLocation(splat_shader, 
				"uPMatrix");

	splat_shader_uPMatrixInverse = glGetUniformLocation(splat_shader, 
				"uPMatrixInverse");

	splat_shader_uViewMat =	glGetUniformLocation(splat_shader, 
				"uViewMat");

	splat_shader_uViewportSize = glGetUniformLocation(splat_shader, 
				"uViewport");

	splat_shader_uTex = glGetUniformLocation(splat_shader, 
				"uTex");

	splat_shader_aVertex =
		glGetAttribLocation	(splat_shader, "aVertex");
	glEnableVertexAttribArray(splat_shader_aVertex);

	splat_shader_aColor =
		glGetAttribLocation	(splat_shader, "aColor");
	glEnableVertexAttribArray(splat_shader_aColor);


	// TEXTURE




    int x,y,n;
    unsigned char *data = stbi_load_from_memory(
    	res_file(noise_png), res_size(noise_png)
    	, &x, &y, &n, 0
    );

	glGenTextures(1, &splat_texture);
	glBindTexture(GL_TEXTURE_2D, splat_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);



	// END TEXTURE




	last_time = ctx_time();

	renderer = (char*)glGetString(GL_RENDERER);

	pthread_create( &mesher_thread, NULL, mesher_loop, NULL);

	return 0;
}


void _ss2ws(
	uint16_t x,
	uint16_t max_x,
	uint16_t y,
	uint16_t max_y,
	mat4 mat, // Inverse projection matrix
	vec3 ray
){
	
	float ray_clip[] = {
		 (x/(float)max_x) * 2.0f - 1.0f,
		-(y/(float)max_y) * 2.0f + 1.0f,
		-1.0f, 1.0f
	};
	
	glm_mat4_mulv3( mat, ray_clip, 1.0f, ray );
	glm_vec_norm(ray);

}


void game_tick(){

	double now = ctx_time();
	double delta = now - last_time;

	glUseProgram(splat_shader);
	glBindVertexArray(splat_vao);
	
	glClearColor( 0.5, 0.5, 0.7, 1.0  );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable( GL_PROGRAM_POINT_SIZE  );

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	int width, height;
	ctx_get_window_size( &width, &height  );
	

	// CAMERA POISITION AND STUFF

	double cur_x, cur_y;
	ctx_cursor_movement( &cur_x, &cur_y  );
	cam_yaw 	+= cur_x*sensitivity;
	cam_pitch 	+= cur_y*sensitivity;
	
	if( cam_pitch>90   ) cam_pitch=90;
	if( cam_pitch<-90  ) cam_pitch=-90;

	float pitch = 	glm_rad(-cam_pitch);
	float yaw = 	glm_rad(-cam_yaw);

	float s = delta*speed;

	float dir[] = {
		cosf(pitch) * sinf(yaw),
		sinf(-pitch),
		cosf(pitch) * cosf(yaw)
	};

	float right[] = {
		sinf(yaw - 3.14/2), 
		0,
		cosf(yaw - 3.14/2)
	};

		
	int vx = 0, vy = 0;	
	ctx_movement_vec( &vx, &vy );
	
	for( int i = 0; i < 3; i++  )
		cam_pos[i] += ( dir[i]*vx + right[i]*vy )*s;
	

	mat4 projection;
	glm_perspective( 80, width/(float)height, 0.1, 4096*2, projection  );
	glm_rotate( projection, glm_rad(cam_pitch), (vec3){1, 0, 0} );
	glm_rotate( projection, glm_rad(cam_yaw)  , (vec3){0, 1, 0} );

	mat4 ip;
	glm_mat4_inv( projection, ip  );

	mat4 view;
	glm_mat4_identity(view);
	glm_translate(view, cam_pos);

	mat4 mvp;
	glm_mul(projection, view , mvp);

	glUniformMatrix4fv(
		splat_shader_uPMatrix, 
		1, GL_FALSE, 
		(const GLfloat*) projection
	);

	glUniformMatrix4fv(
		splat_shader_uPMatrixInverse, 
		1, GL_FALSE, 
		(const GLfloat*) ip
	);

	glUniformMatrix4fv(
		splat_shader_uViewMat, 
		1, GL_FALSE, 
		(const GLfloat*) view
	);

	glUniform2f(
		splat_shader_uViewportSize, 
		(float)width, (float)height
	);

	glActiveTexture(	GL_TEXTURE0 );
	glBindTexture(		GL_TEXTURE_2D, splat_texture );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );


	// FRUSTUM CULL STUFF
	// used to cull nearby stuff
	mat4 near_projection;
	glm_perspective( 80, width/(float)height, 0.1, 8, near_projection  );
	glm_rotate( near_projection, glm_rad(cam_pitch), (vec3){1, 0, 0} );
	glm_rotate( near_projection, glm_rad(cam_yaw)  , (vec3){0, 1, 0} );
	mat4 near_mvp;
	glm_mul(near_projection, view , near_mvp);
	float planes[gfx_fcull_planes_size()];
	gfx_fcull_extract_planes( (float*)mvp, planes );
	float near_planes[gfx_fcull_planes_size()];
	gfx_fcull_extract_planes( (float*)near_mvp, near_planes );

	uint32_t item_count = 0;

	glDisable( GL_CULL_FACE );

	for( int i = 0; i < set->count; i++  ){
		struct ChunkMD *c = &set->chunks[i];
	
		// UPLOAD LOCAL BUFFER TO GPU
		if( c->gl_vbo_local != NULL ){
		
			if( c->gl_vbo == 0  ) glGenBuffers(1, &c->gl_vbo );
			
			glBindBuffer( GL_ARRAY_BUFFER, c->gl_vbo );

			glBufferData(
				GL_ARRAY_BUFFER, c->gl_vbo_local_items *sizeof(int16_t),
				c->gl_vbo_local, GL_STATIC_DRAW
			);

			mem_free(c->gl_vbo_local);
			c->gl_vbo_items = c->gl_vbo_local_items;
			c->gl_vbo_local = NULL;

			c->gl_vbo_type = c->gl_vbo_local_type;
		}

		if( c->gl_vbo == 0 ) continue;

		float cwp[3];

		for (int i = 0; i < 3; ++i)
			cwp[i] = -(c->offset[i] * set->root + set->root/2.0f);

		if( !gfx_fcull(planes, cwp, set->root*0.75) ) continue;

		c->gl_vbo_preferred_type = gfx_fcull(near_planes, cwp, set->root*0.75);

		if(c->gl_vbo_preferred_type != c->gl_vbo_type) continue;

		// RENDER

		glBindBuffer(GL_ARRAY_BUFFER, c->gl_vbo);

		glVertexAttribPointer(
			splat_shader_aVertex, 3, GL_SHORT, GL_FALSE, 
			4*sizeof(int16_t),
			NULL
		);
		
		glVertexAttribPointer(
			splat_shader_aColor, 1, GL_SHORT, GL_FALSE, 
			4*sizeof(int16_t), 
			(const void*) (3*sizeof(int16_t))
		);

		glDrawArrays( GL_POINTS, 0, c->gl_vbo_items/4);
		//glDrawArrays( GL_TRIANGLES, 0, c->gl_vbo_items/4);

		item_count += c->gl_vbo_items/4;
	}
	

	// DRAW AABB WIREFRAMES
	if(show_chunks)
	for( int i = 0; i < set->count; i++  ){
		
		struct ChunkMD *c = &set->chunks[i];

		float min[3];
		float max[3];
		for (int i = 0; i < 3; ++i)
		{
			min[i] = c->offset[i]*set->root - cam_pos[i]+0.1;
			max[i] = (c->offset[i]+1)*set->root  - cam_pos[i]-0.1;
		}

		gfx_aabb_push( 
			min, max
		);
	}
	gfx_aabb_draw( 
		(float*)projection,
		(float[]){1.0, 0.0, 0.0, 1.0}
	);

	
	int _break = ctx_input_break_block();
	int _place  = ctx_input_place_block();

	// RAYCAST
	if( (_break) ||
		(_place)
	 ){
		vec3 ray;

		_ss2ws(1,2,1,2,
			ip,
			ray
		);	

		glm_vec_inv(ray);

		uint32_t hitcoord[3] = {0};
		 int8_t normal[3] = {0};

		Voxel v2 = chunkset_edit_raycast_until_solid(
			set,
			cam_pos,
			ray,		
			hitcoord,
			normal
		);
		if( v2>0 && _break  ){
			chunkset_edit_write(set, hitcoord, 0);
		}
		else if( v2>0 && _place  ){
			for( int i = 0; i < 3; i++  )
				hitcoord[i]+=normal[i];
			chunkset_edit_write(set, hitcoord, 1);
		}
	}


	// INFO

	int w, h;
	ctx_get_window_size( &w, &h );
	float sx = 2.0 / w;
	float sy = 2.0 / h;

	float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	glClear( GL_DEPTH_BUFFER_BIT  );


	static char charbuf[512];

	snprintf( charbuf, 512, 	
			"%i FPS %.1fms\n%s %s.%s (c) kosshi.net\n%s\n%.2f Bv/s\n"
			"Scene: %.2f M\n"
		//	"VRAM %.2f MB\n"
			"%li MB Dynamic\n"
			"%.3f %.3f %.3f",
		
		(int)(1.0/delta), delta*1000,
		PROJECT_NAME, VERSION_MAJOR, VERSION_MINOR,
		renderer,
		1/delta * item_count / 1000000000,
		item_count / 1000000.0f,
		mem_dynamic_allocation() / 1024 / 1024,
		cam_pos[0], cam_pos[1], cam_pos[2]
	);

	gfx_text_draw( 
		charbuf,
		-1 + 8 * sx,
		 1 - 20 * sy,
		sx, sy,
		color, FONT_SIZE
	);

	gfx_crosshair_draw();

	last_time = now;

	ctx_swap_buffers();

}


