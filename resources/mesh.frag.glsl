#version 330

uniform vec2 uViewport;
uniform sampler2D uTex;

uniform float u_far;

in vec3 vColor;
in vec3 vNormal;
in vec2 vUV;
in float vDepth;
flat in int vDiamond;

out vec4 out_Color;

in float ndotl;

void main(void) {
	
	//vec3 light = normalize( vec3(0.0, -1.0, 0.0)  );
	//vec3 light = normalize( vec3(-1.0, -1.0, -1.0)  );
	vec3 light = normalize( vec3(1.0, -1.0, 0.0)  );

	vec3 r = -vNormal;
	//vec3 r = reflect( ray, vNormal );



	vec3 color;

	

	//s = ndotl*s;

	//color = vColor * max( max(0.7, /*ndotl*/ 1.0), sign( dot(r, light) ) );
	
	color = vColor;

	//if(ndotl < 0.5)
	if ( !bool( vDiamond >> int( vUV.x < vUV.y ) & 1) )
		color = vColor*0.6;

	if( vNormal.y < -0.9 
	 || vNormal.x >  0.9 )
		color = vColor*0.6;

	

	gl_FragDepth = vDepth;

	float fog_start = 256.0/u_far;
	float fog = pow( max(gl_FragDepth-fog_start,0.0001), 0.5);

	out_Color = vec4( mix( 
		color, 
		vec3(0.5,0.5,0.7) ,
		(fog)
	), 1.0 );

}
