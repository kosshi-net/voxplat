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
#include "threadpool.h"

#include "cpp/noise.h"

#include <unistd.h>
#include <string.h>
#include <pthread.h>


/*
 * This file contains mostly temporary code that hasn't yet found its place in
 * a dedicated module. Tread with care. Main game loop and init is done here.
 * 
 * If you are looking for documentation, chunkset.h has some
 */


double last_time;
int capped = ' ';

double last_fps_update = 0;
int fps_counter = 0;
int fps = 0;

float last_raytrace = 0;

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

// )=

void command_sensitivity( int argc, char **argv ){
	if(argc == 2) cfg_get()->sensitivity = atof(argv[1]);
	logf_info("%f", cfg_get()->sensitivity);
}
void command_speed( int argc, char **argv ){
	if(argc == 2) cfg_get()->speed = atof(argv[1]);
	logf_info("%f", cfg_get()->speed);
}

void command_fov( int argc, char **argv ){
	if(argc == 2) cfg_get()->fov = atof(argv[1]);
	logf_info("%f", cfg_get()->fov);
}

void command_show_chunks( int argc, char **argv ){
	char *r = &cfg_get()->debug_show_chunk_borders;
	*r = !*r;
	logf_info("%i", *r);
}

void command_debug_freeze_lod( int argc, char **argv ){
	char *r = &cfg_get()->debug_freeze_lod;
	*r = !*r;
	logf_info("%i", *r);
}

void command_held_voxel( int argc, char **argv ){
	if(argc == 2)   cfg_get()->brush_color = atoi(argv[1]);
	logf_info("%i", cfg_get()->brush_color);
}

void command_draw_distance( int argc, char **argv ){
	if(argc == 2) cfg_get()->draw_distance = parse_long(argv[1]);
	logf_info("%i", cfg_get()->draw_distance);
}

void command_brush_size( int argc, char **argv ){
	if(argc == 2)   cfg_get()->brush_size = atoi(argv[1]);
	logf_info("%i", cfg_get()->brush_size);
}

void command_random_color( int argc, char **argv ){
	char *r = &cfg_get()->brush_color_random;
	*r = !*r;
	logf_info("%i", *r);
}

void command_freeze_frustum( int argc, char **argv ){
	char *r = &cfg_get()->debug_freeze_frustum;
	*r = !*r;
	logf_info("%i", *r);
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






struct TaskArg_brush {
	struct ChunkSet *set;
	float ray[3];
	float origin[3];
	uint32_t brush_size;
	Voxel brush_voxel;
};

void task_brush( void *a ){


	struct TaskArg_brush *arg = a;

	//glm_vec_inv(ray);

	int32_t hitcoord[3] = {0};
	int8_t normal[3] = {0};

	Voxel hit_voxel = chunkset_edit_raycast_until_solid(
		arg->set,
		arg->origin,
		arg->ray,		
		(uint32_t*)hitcoord,
		normal
	);
	if( hit_voxel < 1 ) return;

	if( arg->brush_voxel ) 
		for( int i = 0; i < 3; i++  ) hitcoord[i]+=normal[i];


	if( arg->brush_size == 1 )
		chunkset_edit_write( arg->set, 
			(uint32_t*)hitcoord, arg->brush_voxel 
		);
	else
		chunkset_edit_sphere( 
			arg->set, hitcoord, 
			cfg_get()->brush_size, arg->brush_voxel 
		);	

}






const char *renderer;

int game_init(){

	shell_bind_command("sensitivity", 			&command_sensitivity);
	shell_bind_command("speed", 				&command_speed);
	//shell_bind_command("fov", 					&command_fov);

	shell_bind_command("chunkset_edit", 		&command_chunkset_edit);

	shell_bind_command("show_octree",			&command_show_chunks);
	shell_bind_command("freeze_lod",			&command_debug_freeze_lod);

	shell_bind_command("brush_size", 			&command_brush_size);
	shell_bind_command("brush_color",		 	&command_held_voxel);
	shell_bind_command("brush_random_color",	&command_random_color);

	shell_bind_command("draw_distance",			&command_draw_distance);
	shell_bind_command("freeze_frustum",		&command_freeze_frustum);

	

	gfx_vsplat_init();
	gfx_vmesh_init();

	// Create the world
	set = chunkset_create( 
		cfg_get()->chunk_size, cfg_get()->world_size
	);
	chunkset_clear( set ); 
	chunkset_create_shadow_map( set );
	chunkset_gen( set );
	cam.yaw = 90.0+45.0;
	cam.location[0] = 64.0;
	cam.location[1] = 128.0;
	cam.location[2] = 64.0;

	cam.fov = cfg_get()->fov;
	cam.far_plane = 1024*2;

	renderer = (char*)glGetString(GL_RENDERER);

	pthread_create( &mesher_thread, NULL, mesher_loop, NULL);

	last_time = ctx_time();
	return 0;

}




uint32_t 		 	 queue_misc[0xFFFF];
uint32_t 			 queue_misc_count = 0;

struct GeometrySVL 	*queue_gsvl[0xFFFF];
uint32_t 			 queue_gsvl_count = 0;

struct GeometryMesh	*queue_mesh[0xFFFF];
uint32_t 			 queue_mesh_count = 0;

uint32_t 			 debug_walks = 0;



void push_octree_aabb( uint16_t *octree, uint8_t lod ){
	float min[3];
	float max[3];

	for (int i = 0; i < 3; ++i){
		min[i] = octree[i]     << lod;
		max[i] = (octree[i]+1) << lod;
	}

	gfx_aabb_push( 
		min, max
	);
}

#define MIN(a,b) (((a)<(b))?(a):(b))

void octree_cull(
	struct ChunkSet *set,
	uint8_t lod,
	uint8_t x,
	uint8_t y,
	uint8_t z
	//struct ChunkMD **queue
){
	debug_walks++;

	uint16_t ws_root = set->root<<lod;

	float center[] = {
		(x*ws_root) + ( ws_root/2 ),
		(y*ws_root) + ( ws_root/2 ),
		(z*ws_root) + ( ws_root/2 )
	};
	//printf("%f %f %f\n", center[0], center[1] , center[2]);


	if( (x<<lod) >= set->max[0] ) return;
	if( (y<<lod) >= set->max[1] ) return;
	if( (z<<lod) >= set->max[2] ) return;


	if( !gfx_fcull_visible(
		cam.frustum_planes, 
		center,
		ws_root*0.85) 
	) return;

	float distance = glm_vec_distance( cam.lod_origin, center );

	//if( distance > (pow(lod,2))*700
	if( distance > lod*(1024)-256 ){
	
		uint8_t octree_bitw[3];
		for (int i = 0; i < 3; ++i)
			octree_bitw[i] = set->max_bitw[i] - MIN(lod, set->max_bitw[i]);

		struct GeometrySVL *g = &set->gsvl [ lod ][  
			flatten3( (uint16_t[]){x,y,z}, octree_bitw )  
		];

		if( lod == 0 ){
			uint32_t index = flatten3( (uint16_t[]){x,y,z}, set->max_bitw ) ;
			struct GeometryMesh *m = &set->gmesh[ index ];
			if( m->vbo ) {
				queue_mesh[queue_mesh_count++] = m;
				queue_misc[queue_misc_count++] = index;
				return; 
			}
			// if no mesh vbo, fall thru and use SVL
		}

		if( !g->vbo ) return;

		if( cfg_get()->debug_show_chunk_borders )
		push_octree_aabb( (uint16_t[]){x,y,z}, lod+set->root_bitw );

		queue_gsvl[queue_gsvl_count++] = g;

		return;
	}

	lod--;
	for (int nx = 0; nx < 2; ++nx)
	for (int ny = 0; ny < 2; ++ny)
	for (int nz = 0; nz < 2; ++nz){
		octree_cull( set, lod, 
			(x * 2) + nx, 
			(y * 2) + ny,
			(z * 2) + nz
		);
	}
}

float blocked_for = 0;
float unblock_until = 0;

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

	cam.far_plane = cfg_get()->draw_distance;
	cam.fov =  cfg_get()->fov;

	double cur_x, cur_y;
	ctx_cursor_movement( &cur_x, &cur_y  );
	cam.yaw 	-= cur_x * cfg_get()->sensitivity;
	cam.pitch 	-= cur_y * cfg_get()->sensitivity;
	
	if( cam.pitch > 90   ) cam.pitch=90;
	if( cam.pitch < -90  ) cam.pitch=-90;

	float pitch = 	glm_rad(-cam.pitch);
	float yaw = 	glm_rad(-cam.yaw);

	float s = delta*cfg_get()->speed;

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

	if( !cfg_get()->debug_freeze_frustum )
	gfx_fcull_extract_planes( 
		(float*)cam.view_projection,
		cam.frustum_planes
	);

	if( !cfg_get()->debug_freeze_lod ) {
		memcpy( cam.lod_origin, cam.location, sizeof(float)*3 );
	}


	//
	// Create draw queue and set LOD levels
	//

	float cull_time = ctx_time();

	uint32_t splat_count = 0;
	uint32_t vertex_count = 0;


	queue_gsvl_count = 0;
	queue_mesh_count = 0;
	queue_misc_count = 0;

	debug_walks=0;
	uint16_t cs[3];
	//for (cs[0] = 0; cs[0] < (set->max[0]>>4); ++cs[0])
	//for (cs[1] = 0; cs[1] < (set->max[0]>>4); ++cs[1])
	//for (cs[2] = 0; cs[2] < (set->max[2]>>4); ++cs[2]){
	for (cs[0] = 0; cs[0] < (set->max[0]>>4)+1; ++cs[0])
	for (cs[1] = 0; cs[1] < (set->max[1]>>4)+1; ++cs[1])
	for (cs[2] = 0; cs[2] < (set->max[2]>>4)+1; ++cs[2]){
		octree_cull( set, 4, cs[0], cs[1], cs[2] );
	}

	cull_time = ctx_time() - cull_time;

	//
	// Render!
	//

	glClearColor( 0.5, 0.5, 0.7, 1.0  );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	gfx_sky_draw(&cam);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable( GL_PROGRAM_POINT_SIZE  );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glDisable( GL_CULL_FACE );
	glCullFace( GL_BACK );


	gfx_vmesh_draw(
		&cam,
		set,
		queue_mesh,
		queue_mesh_count,
		&vertex_count
	);

	gfx_svl_draw(
		&cam,
		set,
		queue_gsvl,
		queue_gsvl_count,
		&splat_count
	);

	if( cfg_get()->debug_show_chunk_borders ){
		gfx_aabb_draw( 
			&cam,
			(float[]){0.3, 0.3, 0.3, 1.0}
		);
			
		for( int i = 0; i <     queue_misc_count; i++  ){
			struct ChunkMD *c = &set->chunks[ queue_misc[i] ];

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
	// Upload
	//
	// Implements delaying uploads if multiple chunks are changing nearby
	// to avoid voids between chunks

	#define MAX_UPLOADS 16

	uint32_t qm[MAX_UPLOADS];
	uint32_t qm_c = 0; 

	uint32_t qs [MAX_UPLOADS];
	uint32_t qs_c = 0;

	uint8_t block_upload = 0;

	uint32_t geometry_items = 0;

	for( int i = 0; i < set->count; i++  ){
		struct ChunkMD *c = &set->chunks[i];

		geometry_items += c->svl_items_total;

		float chunk_center_ws[3];
		for (int i = 0; i < 3; ++i) 
			chunk_center_ws[i] = (c->offset[i] << set->root_bitw) + (set->root/2);

		float distance = glm_vec_distance( chunk_center_ws, cam.lod_origin );

		uint8_t new_value = distance < 128;


		// help
		if( c->make_mesh != new_value ){
			c->make_mesh = new_value;
			c->remesh = 1;
		}
		if( distance > 256 ) {
			if( c->svl_dirty ) {
				gfx_update_svl( set, i );
				if( c->mesh_has_vbo ) {
					gfx_delete_mesh( set, i );
					c->mesh_has_vbo = 0;
				}
			}
			if( c->mesh_dirty ) {
				if( c->no_geometry ) {
					gfx_delete_mesh( set, i );
				} else {
					gfx_update_mesh( set, i );
					c->mesh_has_vbo = 1;
				}
			}
		} else {
			if( c->dirty || c->changing ) block_upload = 1;
			if( c->svl_dirty && qs_c < MAX_UPLOADS )
				qs[ qs_c++ ] = i;
			if( c->mesh_dirty && qm_c < MAX_UPLOADS )
				qm[ qm_c++ ] = i;
		}
	}

	// If blocked for too long, disable blocking
	if((block_upload && blocked_for > 0.1) 
	|| (block_upload && unblock_until > now ) 
	){
		blocked_for = 0;
		block_upload = 0;
		unblock_until = now + 0.1;
	}

	if(!block_upload){
		for (int i = 0; i < qs_c; ++i){
			struct ChunkMD *c = &set->chunks[qs[i]];
			gfx_update_svl( set, qs[i] );
			if( c->mesh_has_vbo ) {
				gfx_delete_mesh( set, qs[i] );
				c->mesh_has_vbo = 0;
			}
			
		}
		for (int i = 0; i < qm_c; ++i){
			struct ChunkMD *c = &set->chunks[qm[i]];
			if( c->no_geometry ) {
				gfx_delete_mesh( set, qm[i] );
			} else {
				gfx_update_mesh( set, qm[i] );
				c->mesh_has_vbo = 1;
			}
		}
	}else{
		blocked_for += delta;
	}


	//
	// World editing 
	//

	if( cfg_get()->brush_color_random )
		cfg_get()->brush_color = 63*noise_randf();
	
	int _break = ctx_input_break_block();
	int _place  = ctx_input_place_block();

	if( (_break || _place) 
		&& last_raytrace + (1.0f/60.0f) < now
	){

		last_raytrace = now;
		vec3 ray;

		_ss2ws(1,2,1,2,
			cam.inv_proj_view_rot,
			ray
		);	

		// Send task to the thread pool
		struct TaskArg_brush *args = mem_alloc(sizeof (struct TaskArg_brush));
		args->set = set;
		memcpy( args->ray, ray, sizeof(ray) );
		memcpy( args->origin, cam.location, sizeof(args->origin) );
		args->brush_size = cfg_get()->brush_size;
		args->brush_voxel = _break ? 0 : cfg_get()->brush_color;

		threadpool_task( &task_brush, args );

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
			"Queue    %i / %i (%i nodes in %.1f ms)\n"
			"RAM      %li / %li MB (SVL %li MB)\n"
			""
			"X: %.2f\n"
			"Y: %.2f\n"
			"Z: %.2f\n",
		
		fps, capped, delta*1000,
		PROJECT_NAME, VERSION_MAJOR, VERSION_MINOR,
		renderer,
		splat_count / 1000000.0f,
		1/delta * splat_count / 1000000000,
		vertex_count / 1000000.0f,
		queue_gsvl_count, set->count, debug_walks, cull_time*1000,

		mem_dynamic_allocation() >> 20,
		cfg_get()->heap >> 20,

		geometry_items * sizeof(uint16_t) >> 20,

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


