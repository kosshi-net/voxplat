#version 330

uniform vec2 uViewport;
uniform sampler2D uTex;

uniform float u_far;

in vec3 vColor;
in vec3 vNormal;
in float vDepth;

out vec4 out_Color;


void main(void) {
	
	vec3 light = normalize( vec3(0.0, -1.0, 0.0)  );

	vec3 r = -vNormal;
	vec3 color =  vColor * ( dot( r, light) / 4.0 + 0.75 );
	//vec3 color = vColor;

	gl_FragDepth = vDepth;

	float fog = pow(gl_FragDepth,0.5);

	out_Color = vec4( mix( 
		color, 
		vec3(0.5,0.5,0.7) ,
		(fog)
	), 1.0 );

}
