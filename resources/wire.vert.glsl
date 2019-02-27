#version 120

attribute vec4 aVertex;

uniform mat4 u_projection;
uniform mat4 u_view_rotation;
uniform mat4 u_view_translation;

varying float vDepth;
varying vec4 vPos;

void main(void) {
	vPos = u_view_translation*aVertex;

	gl_Position = ( u_projection) * ( aVertex );
	//uDepth = gl_Position.z*gl_Position.w*(2048.0*2.0);
	//uDepth = length(aVertex)/(4096*1.0);
	//vPos = aVertex.xyz;
} 