#define __FILENAME__ "gfx/camera.c"
#include "gfx/camera.h"
#include "mem.h"
#include "ctx.h"
#include "event.h"

#include <cglm/cglm.h>


// DO NOT USE THIS FUNCTION
// cglm for whatever reason segfaults with mat4_idendity(cam->view)
// just put the cam on stack and itll work
struct Camera *gfx_camera_create(){

	struct Camera *cam = mem_alloc( sizeof( struct Camera ) );

	logf_info("Camera %lu", sizeof( struct Camera ));

	return cam;

}