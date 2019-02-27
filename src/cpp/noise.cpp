#include <FastNoise.h>

#include <stdio.h>
#include <stdlib.h>

FastNoise splx; // Create a FastNoise object

bool init = false;

extern "C" void noise_init(void){
	
	splx.SetNoiseType(FastNoise::Simplex); // Set the desired noise type
	splx.SetSeed(rand());

	init=true;	
}

extern "C" float noise_randf(void)
{
	return rand() / (float)RAND_MAX * 2.0 - 1.0;
}

extern "C" float noise_simplex(float x, float y, float z ){
		
	return splx.GetNoise(x,y,z);

}
