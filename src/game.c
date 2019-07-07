#define __FILENAME__ "game.c"

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <glad/glad.h>
#include <cglm/cglm.h>

#include <config.h>
#include "cfg.h"

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
#include "gfx/vsplat.h"
#include "gfx/vmesh.h"
#include "gfx/camera.h"
#include "gfx/sky.h"

#include "mem.h"
#include "res.h"
#include "chunkset.h"
#include "chunkset/edit.h"
#include "chunkset/gen.h"


#include "cpp/noise.h"

#include <unistd.h>
#include <string.h>
#include <pthread.h>


/*
 * This file contains mostly temporary code that hasn't yet found its place in
 * a dedicated module. Tread with care. Main game loop and init is done here.
 */


double last_time;
int capped = ' ';

double last_fps_update = 0;
int fps_counter = 0;
int fps = 0;

struct Camera cam;

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


float sensitivity = 0.15;
void command_sensitivity( int argc, char **argv ){
	if(argc == 2){
		float temp = atof(argv[1]);
		if( temp != 0.0 )
			sensitivity = temp;
	}
	logf_info("%f", sensitivity);
}

float speed = 75;
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

int debug_mark_near = 1;
void command_debug_mark_near( int argc, char **argv ){
	debug_mark_near = !debug_mark_near;
	logf_info("%i", debug_mark_near);
}

Voxel held_voxel = 63;
void command_held_voxel( int argc, char **argv ){
	if(argc == 2){
		float temp = atoi(argv[1]);
		held_voxel = temp;
	}
	logf_info("%i", held_voxel);
}

uint8_t brush_size = 5;
void command_brush_size( int argc, char **argv ){
	if(argc == 2){
		float temp = atoi(argv[1]);
		brush_size = temp;
	}
	logf_info("%i", brush_size);
}

void command_memory_debug( int argc, char **argv ){
	mem_log_debug();
}

void command_export( int argc, char **argv ){
	FILE *fptr;
	fptr = fopen("default.bin","wb");

	char magic = (char)137;

	//char *mid = (char*)0xDEAD;

	fwrite( &magic, 1, 1, fptr );
	fwrite( "VOXPLAT", 7, 1, fptr );
	fwrite( &set->root_bitw, 1, 1, fptr );
	fwrite( set->max_bitw, 3, 1, fptr );

	for (uint32_t i = 0; i < set->count; ++i){
		struct ChunkMD *c = &set->chunks[i];

		uint32_t j = 0;
		for(; c->rle[j]; j+=2);
		j+=2;
		fwrite( c->rle, j, 1, fptr );
		//fwrite( "\0\0", 4, 1, fptr );
	}

	fwrite( set->shadow_map, set->shadow_map_length, 1, fptr );

	fclose(fptr);
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

const char *renderer;

int game_init(){

	shell_bind_command("sensitivity", 		&command_sensitivity);
	shell_bind_command("speed", 			&command_speed);
	shell_bind_command("chunkset_edit", 	&command_chunkset_edit);
	shell_bind_command("toggle_chunk_aabb",	&command_show_chunks);
	shell_bind_command("toggle_lod",		&command_debug_mark_near);
	shell_bind_command("color",		 		&command_held_voxel);
	shell_bind_command("memory_debug", 		&command_memory_debug);
	shell_bind_command("export", 			&command_export);
	shell_bind_command("brush_size", 		&command_brush_size);

	gfx_vsplat_init();
	gfx_vmesh_init();

	/* 
		Define the size of the world and chunks in power of twos.
		5 = 32
		6 = 64
		7 = 128
		etc

		Good options:
		6, (uint8_t[]){5,2,5} 	2048x256x2048 with 64 chunks
		6, (uint8_t[]){6,2,6} 	4096x256x4096 with 64 chunks
		7, (uint8_t[]){5,1,5} 	4096x256x4096 with 128 chunks
	*/
	//set = chunkset_create( 
	//	6, (uint8_t[]){5,2,5}
	//);
	
	set = chunkset_create( 
		cfg_get()->chunk_size, cfg_get()->world_size
	);


	//chunkset_clear_import( set );
	chunkset_clear( set );
	chunkset_create_shadow_map( set );
	chunkset_gen( set );
	
	cam.location[1] = 512;
	cam.fov =  90.0f;
	cam.far_plane = 1024*2;

	renderer = (char*)glGetString(GL_RENDERER);

	pthread_create( &mesher_thread, NULL, mesher_loop, NULL);

	last_time = ctx_time();
	return 0;
}



void game_tick(){

	double now = ctx_time();
	double delta = now - last_time;

	fps_counter++;
	if( now > last_fps_update+0.25 ){
		last_fps_update = now;
		fps = fps_counter*4;
		fps_counter = 0;
	}

	int width, height;
	ctx_get_window_size( &width, &height  );
	glViewport(0,0,width,height);
	
	//
	// Camera position and matrices
	//

	double cur_x, cur_y;
	ctx_cursor_movement( &cur_x, &cur_y  );
	cam.yaw 	-= cur_x*sensitivity;
	cam.pitch 	-= cur_y*sensitivity;
	
	if( cam.pitch > 90   ) cam.pitch=90;
	if( cam.pitch < -90  ) cam.pitch=-90;

	float pitch = 	glm_rad(-cam.pitch);
	float yaw = 	glm_rad(-cam.yaw);

	float s = delta*speed;

	float dir[] = {
		cosf(pitch) * sinf(yaw),
		sinf(-pitch),
		cosf(pitch) * cosf(yaw)
	};

	float right[] = {
		sinf(yaw + 3.14/2), 
		0,
		cosf(yaw + 3.14/2)
	};
		
	int vx = 0, vy = 0;	
	ctx_movement_vec( &vx, &vy );
	
	for( int i = 0; i < 3; i++  )
		cam.location[i] -= ( dir[i]*vx + right[i]*vy )*s;


	//glm_perspective( cam.fov, width/(float)height, 0.1, cam.far_plane, cam.projection );
	glm_perspective( glm_rad(cam.fov), width/(float)height, 0.1, cam.far_plane, cam.projection );

	glm_mat4_inv( cam.projection, cam.inverse_projection  );

	glm_mat4_identity(cam.view);
	glm_mat4_identity(cam.view_rotation);
	glm_mat4_identity(cam.view_translation);

	float location_inverse[3];
	for (int i = 0; i < 3; ++i)
		location_inverse[i] = -cam.location[i];

	glm_translate(cam.view_translation, location_inverse);

	glm_rotate( cam.view_rotation, glm_rad(cam.pitch), (vec3){1, 0, 0} );
	glm_rotate( cam.view_rotation, glm_rad(cam.yaw)  , (vec3){0, 1, 0} );

	glm_mul( 
		cam.view_rotation,
		cam.view_translation,
		cam.view
	 );

	glm_mul( cam.projection, cam.view, cam.view_projection );

	glm_mul( 
		cam.projection,
		cam.view_rotation,
		cam.inv_proj_view_rot
	);
	glm_mat4_inv(
		cam.inv_proj_view_rot,
		cam.inv_proj_view_rot
	);

	gfx_fcull_extract_planes( 
		(float*)cam.view_projection,
		cam.frustum_planes
	);

	//
	// Create draw queue and set LOD levels
	//

	uint32_t splat_count = 0;
	uint32_t vertex_count = 0;

	struct ChunkMD *chunk_draw_queue[0xFFFF];
	uint32_t 		chunk_draw_queue_count = 0;
//	struct ChunkMD *chunk_near_draw_queue[1024];
//	uint32_t 		chunk_near_draw_queue_count = 0;

	for( int i = 0; i < set->count; i++  ){
		struct ChunkMD *c = &set->chunks[i];

		float cwp[3]; // Chunk world position
		float here[3];
		for (int i = 0; i < 3; ++i){
			cwp[i] = (c->offset[i] * set->root + set->root/2.0f);
			here[i] = cam.location[i];
		}
		float distance = glm_vec_distance( here, cwp );
		if( debug_mark_near ){
			c->lod = (distance > 128.0);
			if((distance > 1024.0)) c->lod = 2;

			// Lod above 2 is broken atm
			//if((distance > 1024.0)) c->lod = 3;
			//if((distance > 2048.0)) c->lod = 3;
			//if((distance > 4096.0)) c->lod = 4;
			//if((distance > cam.far_plane)) c->lod = -1;
		}
		
		if( c->mesher_lock && *c->mesher_lock ) goto push;


		if( c->gl_vbo == 0 && c->gl_vbo_local == 0) 				continue;
		if( c->gl_vbo_local_items == 0 ) 							continue;

		// Frustum cull
		if( distance > cam.far_plane ) 								continue;
		if( !gfx_fcull(cam.frustum_planes, cwp, set->root*0.85) ) 	continue;

		push:
			chunk_draw_queue[chunk_draw_queue_count++] = c;
	}

	//
	// Render!
	//

	glClearColor( 0.5, 0.5, 0.7, 1.0  );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); 

	gfx_sky_draw(&cam);

	gfx_vmesh_draw(
		&cam,
		set,
		chunk_draw_queue,
		chunk_draw_queue_count,
		&vertex_count
	);
	gfx_vsplat_draw(
		&cam,
		set,
		chunk_draw_queue,
		chunk_draw_queue_count,
		&splat_count
	);


	// Draw chunk AABB wireframes
	if(show_chunks){
		for( int i = 0; i < chunk_draw_queue_count; i++  ){
			struct ChunkMD *c = chunk_draw_queue[i];

			if( c->lod > 0 ) continue;

			float min[3];
			float max[3];
			for (int i = 0; i < 3; ++i){
				min[i] = c->offset[i]*set->root + 		0.1;
				max[i] = (c->offset[i]+1)*set->root  - 	0.1;
			}
			gfx_aabb_push( 
				min, max
			);
		}
		gfx_aabb_draw( 
			&cam,
			(float[]){1.0, 1.0, 1.0, 1.0}
		);

		for( int i = 0; i < chunk_draw_queue_count; i++  ){
			struct ChunkMD *c = chunk_draw_queue[i];

			if( c->lod == 0 ) continue;

			float min[3];
			float max[3];
			for (int i = 0; i < 3; ++i){
				min[i] = c->offset[i]*set->root + 		0.1;
				max[i] = (c->offset[i]+1)*set->root  - 	0.1;
			}
			gfx_aabb_push( 
				min, max
			);
		}
		gfx_aabb_draw( 
			&cam,
			(float[]){1.0, 0.0, 1.0, 1.0}
		);
	}

	//
	// World editing 
	//
	
	int _break = ctx_input_break_block();
	int _place  = ctx_input_place_block();

	if( (_break) ||
		(_place)
	 ){
		vec3 ray;

		_ss2ws(1,2,1,2,
			cam.inv_proj_view_rot,
			ray
		);	

		//glm_vec_inv(ray);

		uint32_t hitcoord[3] = {0};
		 int8_t normal[3] = {0};

		Voxel v2 = chunkset_edit_raycast_until_solid(
			set,
			cam.location,
			ray,		
			hitcoord,
			normal
		);
		if( v2>0 && _break  ){
			uint32_t u[3];
			for( u[0] = hitcoord[0]-brush_size; u[0] < hitcoord[0]+brush_size; u[0]++ )
			for( u[1] = hitcoord[1]-brush_size; u[1] < hitcoord[1]+brush_size; u[1]++ )
			for( u[2] = hitcoord[2]-brush_size; u[2] < hitcoord[2]+brush_size; u[2]++ ){
				float _a[3], _b[3];
				for (int i = 0; i < 3; ++i){	
					_a[i] = (float)u[i];
					_b[i] = (float)hitcoord[i];
				}
				if (glm_vec_distance(_a, _b) < brush_size){
					chunkset_edit_write(set, u, 0);
					shadow_break_update(set, u);
				}
			}
		}
		else if( v2>0 && _place  ){
			for( int i = 0; i < 3; i++  )
				hitcoord[i]+=normal[i];
			chunkset_edit_write(set, hitcoord, held_voxel);
			shadow_place_update(set, hitcoord);
		}
	}

	//
	// Info
	//

	int w, h;
	ctx_get_window_size( &w, &h );
	float sx = 2.0 / w;
	float sy = 2.0 / h;

	float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};

	glClear( GL_DEPTH_BUFFER_BIT  );



	static char charbuf[512];

	snprintf( charbuf, 512, 	
			"%i%c FPS  %.1fms\n"
			"%s %s.%s (c) kosshi.net\n%s\n"
			"Splatted %.2f M voxels (%.2f Bv/s)\n"
			"Other    %.2f M vertices\n"
			"Queue    %i / %i\n"
		//	"VRAM %.2f MB\n"
			"%li MB Dynamic\n"
			"%.3f %.3f %.3f",
		
		fps, capped, delta*1000,
		PROJECT_NAME, VERSION_MAJOR, VERSION_MINOR,
		renderer,
		splat_count / 1000000.0f,
		1/delta * splat_count / 1000000000,
		vertex_count / 1000000.0f,
		chunk_draw_queue_count, set->count,
		mem_dynamic_allocation() / 1024 / 1024,
		cam.location[0], cam.location[1], cam.location[2]
	);

	gfx_text_draw( 
		charbuf,
		-1 + 8 * sx,
		 1 - 20 * sy,
		sx, sy,
		color, FONT_SIZE
	);

	gfx_crosshair_draw();

	//ctx_swap_buffers();
	glFlush();

	// Frame limit
	capped = ' ';
	double worked = ctx_time();
	if( worked-now < 0.0032 && fps > 280) {
		while( ctx_time() < now+0.0032 ) usleep(50);
		capped = '*';
	}

	last_time = now;
}


