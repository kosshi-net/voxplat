#version 330

uniform mat4 u_inv_proj_view_rot;

uniform float u_far;

uniform vec2 uViewport;
uniform sampler2D uTex;

in vec3 vColor;
in vec4 vVert;

uniform float u_lod;

out vec4 out_Color;

varying float ndotl;

// ro = ray origin
// rd = ray direction
// minV = aabb min
// maxV = aabb max


vec2 AABBIntersect(vec3 ro, vec3 rd, vec3 minV, vec3 maxV)
{
    vec3 invR = 1.0 / rd;

    float t0, t1;

    vec3 tbot = (minV - ro) * invR;
    vec3 ttop = (maxV - ro) * invR;

    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);


    vec2 t = max(tmin.xx, tmin.yz);

    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);

    return vec2(t0, t1); 
	// if (t0 <= t1) { did hit } else { did not hit }
}

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

	vec3 vxl = vVert.xyz;

	//if( mod( floor(gl_FragCoord.y/gl_FragCoord.w*uViewport.y), 2) == 0 ) discard;
	

	vec3 ray = ss2wsVec(
		gl_FragCoord.x/uViewport.x, 
		gl_FragCoord.y/uViewport.y
	);
	

	vec2 result = AABBIntersect( 
		vec3(0.0), ray, 
		vec3( vxl-0.5*u_lod),
		vec3( vxl+0.5*u_lod)
	);

	//if( ! (result.x<=result.y) ) discard;

	if( ! (result.x<=result.y) ) {

		//gl_FragDepth = 0.99;
		//out_Color = vec4(1);
		discard;
	} else {

	// CALCULATE NORMAL
	
	vec3 hit = vxl - ray*result.x;
	vec3 absHit = abs(hit); // Relative to voxel center
	
	float max = max( max( absHit.x, absHit.y), absHit.z  );

	vec3 normal = vec3(
		float(absHit.x == max), 
		float(absHit.y == max), 
		float(absHit.z == max) 
	)*sign(hit);

	// UV
	vec2 uv;
	if( normal.y != 0.0 ){
		uv.y = hit.x;
		uv.x = hit.z*normal.y;
	}
	if( normal.x != 0.0 ){
		uv.y = -hit.y;
		uv.x = hit.z*normal.x;
	}
	if( normal.z != 0.0 ){
		uv.y = -hit.y;
		uv.x = hit.x*-normal.z;
	}

	uv = uv + 0.5;

	// COLOR

	//vec3 r = reflect( ray, normal );
	vec3 r = -normal;
	//vec3 color =  vColor * ( dot( r, light) / 4.0 + 0.75 );
	vec3 color;

	float d = (dot( r, light ));

	if(ndotl == 1.0f){
		//color =  vColor * ( dot( r, light) / 4.0 + 0.75 );
		if( d < 0 )
			color = vColor * 0.7 ;
		else
			color = vColor;
	}
	else{
		color = vColor;
	}
	//vec3 color = vColor;
	//color*=texture(uTex, uv).xyz;
	//vec3 color  = vec3( uv.x, 0.0, uv.y );
	//vec3 color = texture(uTex, uv).xyz;

	// FOG


	gl_FragDepth = result.x/u_far;

	float fog = pow(gl_FragDepth,0.7);
	//float fog = 0;

	out_Color = vec4( mix( 
		color, 
		vec3(0.5,0.5,0.7) ,
		(fog)
	), 1.0 );
	

	}// END DISCARD ELSE

}
