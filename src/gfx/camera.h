#ifndef GFX_CAMERA_H
#define GFX_CAMERA_H 1


#include <cglm/cglm.h>

struct Camera {

	mat4 projection;
	mat4 inverse_projection;

	mat4 view_rotation;
	mat4 view_translation;
	mat4 view;

	mat4 inv_proj_view_rot;

	mat4 view_projection;

	float fov;
	float far_plane; // thanks microsoft, can't make it just far!

	float location[3];
	float lod_origin[3];
	float yaw;
	float pitch;

	float frustum_planes[24];
};

#endif