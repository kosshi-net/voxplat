#version 330

uniform mat4 u_inv_proj_view_rot;
uniform vec2 uViewport;

out vec4 out_Color;

vec3 ss2wsVec(float x, float y){
	vec4 ray_clip = vec4(
		 x * 2.0 - 1.0,
		 y * 2.0 - 1.0,
		-1.0, 1.0
	);

	//vec4 ray = uPMatrixInverse * ray_clip;
	vec4 ray = u_inv_proj_view_rot * ray_clip;

	return normalize(ray.xyz/ray.w);
}

void main(void) {

	//vec3 light = normalize( vec3(0.0, -1.0, 0.0)  );
	vec3 light = normalize( vec3(1.0, 1.0, 1.0)  );
	
	vec3 ray = ss2wsVec(
		gl_FragCoord.x/uViewport.x, 
		gl_FragCoord.y/uViewport.y
	);
	

	float d = (dot( ray, light ))*0.5+0.5;

	out_Color = vec4( 
		mix(
			mix( 
				vec3(0.5,0.5,0.7),
				vec3(0.7, 0.7, 0.8),
				pow(d, 2)
			),
			vec3(1.0, 1.0, 0.95),
			pow(d+0.00025,1000.0)
		)
	, 1.0 );
	

}
