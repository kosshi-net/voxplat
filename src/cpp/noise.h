#ifndef NOISE_H
#define NOISE_H 1

#include <stdlib.h>

float noise_simplex( float, float, float );

float noise_randf(void);

void noise_init(void);

//float noise_u_simplex_f3( float x, float y, float z ){
//	return noise_simplex( x,y,z ) / 2.0 + 0.5 ;
//}
#define noise_u_simplex_f3( x, y, z ) (noise_simplex( x,y,z ) / 2.0 + 0.5)

#endif
