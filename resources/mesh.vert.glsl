#version 330

in vec4 aVertex;
in int aColor;

uniform mat4 u_projection;
uniform mat4 u_view_translation;
uniform float u_far;

out vec3 vColor;
out vec2 vUV;
out vec3 vNormal;
out float vDepth;

out float ndotl;

flat out int vDiamond;


vec3 unpack_color(int c){
	vec3 color;
	color.b = c & 3;
	color.g = c >> 2 & 3;
	color.r = c >> 4 & 3;
	return color / 3;
}

float unpack_ao( int c ){
	float darken = c >> 6 & 3;
	return (min(darken,2)/12);
}

float unpack_shadow( int c ){
	return float(c >> 13 & 1);
}

int unpack_diamond( int c ){
	return (c >> 14 & 3);
}

int unpack_uv( int c ){
	return (c >> 11) & 3;
}

vec3 unpack_normal(int c){
	vec3 lut[] = vec3[6](
		vec3( 1, 0, 0 ),
		vec3( 0, 1, 0 ),
		vec3( 0, 0, 1 ),
		vec3(-1, 0, 0 ),
		vec3( 0,-1, 0 ),
		vec3( 0, 0,-1 )
	);

	int index = (c>>8) & 7;

	return lut[index];
}


void main(void) {

	//vVert = u_view_mat * (aVertex);

	gl_Position = ( u_projection ) * ( aVertex );

	//vDepth = length( aVertex.xyz );
	float d = length(u_view_translation*aVertex);
	vDepth = d/u_far;
	float aow = 1.0-min(d/180.0,1.0);

	ndotl = unpack_shadow(aColor);

	vNormal = unpack_normal(aColor);
	vColor = 
		unpack_color(aColor) *
		max( 1.0 - unpack_shadow(aColor), 0.6) *
		(1.0-unpack_ao(aColor)*aow)
		;
		
	vec2 clut[] = vec2[4](
		vec2( 0, 0 ),
		vec2( 1, 0 ),
		vec2( 1, 1 ),
		vec2( 0, 1 )
	);

	vUV = clut[unpack_uv(aColor)];


	vDiamond = unpack_diamond(aColor);

}

