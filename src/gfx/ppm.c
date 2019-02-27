#include "ppm.h"
#include <stdio.h>

void gfx_util_write_ppm(const int width, const int height, unsigned char*buffer){

	FILE *fp = fopen("file.ppm", "wb"); /* b - binary mode */

	(void) fprintf(fp, "P6\n%d %d\n255\n", width, height);

	for (int i = 0; i < width*height; ++i){
		static unsigned char color[3];
		color[0] = buffer[i];
		color[1] = buffer[i];
		color[2] = buffer[i];
		(void) fwrite(color, 1, 3, fp);
	}
	
	(void) fclose(fp);
};
