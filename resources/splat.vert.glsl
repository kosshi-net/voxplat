#version 330

in vec4 aVertex;
in int aColor;

uniform mat4 u_projection;
uniform mat4 u_view_rotation;
uniform mat4 u_view_translation;

uniform vec2 uViewport;

out vec4 vVert;
out vec3 vColor;

uniform float u_lod;

out float ndotl;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 unpack6bit(int c){
	vec3 color;
	color.b = c & 3;
	color.g = c >> 2 & 3;
	color.r = c >> 4 & 3;
	float shadow = (c >> 6 & 1);
	ndotl = shadow;
	return color / 3 * max(0.7, 1.0-shadow);
}

void main(void) {


	vVert = u_view_translation * (aVertex + vec4(0.5, 0.5, 0.5, 0.0)*u_lod);
	gl_Position = (u_projection * u_view_rotation) * ( vVert );

	float ratio = uViewport.x/uViewport.y;
	float reduce = max(
		abs( gl_Position.x*ratio/gl_Position.w  ),
		abs( gl_Position.y/gl_Position.w  )
	);
	// Following two values directly affect performance.
	reduce += 0.03; 		// Overdraw nearing edges
	float scale = 1.05; 	// Base size
	float size = ( uViewport.y*1.1 ) / gl_Position.z * max(reduce, 1.0);
	gl_PointSize = size * u_lod;

	vColor = unpack6bit(aColor); 
	// * (0.8+( rand(aVertex.xz * rand(aVertex.yz)) )*0.2);

}

