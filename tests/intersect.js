
// ro = ray origin
// rd = ray direction
// minV = aabb min
// maxV = aabb max




function AABBIntersect( ro, rd, minV, maxV  ) {

	let t0, t1;	
	
	let tbot = [], ttop = [], tmin = [], tmax = [], invR = [];

	for( let i = 0; i < 3; i++  ){
		invR[i] = 1.0 / rd[i];
		
		tbot[i] = (minV[i] - ro[i]) * invR[i];

		ttop[i] = (maxV[i] - ro[i]) * invR[i];
		
		tmin[i]	= Math.min( ttop[i], tbot[i]  );
		tmax[i]	= Math.max( ttop[i], tbot[i]  );
	}

	let t = [
		Math.max( tmin[0], tmin[1]  ),
		Math.max( tmin[0], tmin[2]  )
	];

	t0 = Math.max( t[0], t[1] );
	
	t = [
		Math.max( tmax[0], tmax[1]  ),
		Math.max( tmax[0], tmax[2]  )
	];

	t1 = Math.min( t[0], t[1] )
	
	console.log( t0, t1, (t0<=t1) );
	
}





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

    return vec2(t0, t1); // if (t0 <= t1) { did hit } else { did not hit }
}



