#include <stdlib.h>
#include <math.h>


float vec3length( float * v ){
	return sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
};

float vec3dot( float * A, float* B ){
	return (A[0]*B[0])+(A[1]*B[1])+(A[2]*B[2]);
};


size_t gfx_fcull_planes_size(){
	return sizeof(float)*4*6;
}

void gfx_fcull_extract_planes(float* M, float*planes){
	int n = 0;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) 
			planes[n++] = M[3+4*j]-M[4*j+i];	
		for (int j = 0; j < 4; j++) 
			planes[n++] = M[3+4*j]+M[4*j+i];
	}

	for(int i = 0; i < 6; ++i){
		int r = i*4;
		float length = vec3length( &planes[r] );
		planes[r+0] /= length;
		planes[r+1] /= length;
		planes[r+2] /= length;
		planes[r+3] /= length;
	}
}


int gfx_fcull(float*planes, float*loc, float radius){
	for(int i = 0; i < 6; ++i){
		if( vec3dot( &planes[i*4], loc ) + planes[i*4+3] < -radius ){
			return 0;
		}
	}
	return 1;
}
