#include <stdio.h>
#include <stdlib.h>

int main(){

	FILE *fptr;
	fptr = fopen("test.bin","wb");

	fwrite(
		"test",
		4,
		1,
		fptr
	);
	fwrite(
		"test",
		4,
		1,
		fptr
	);

	fclose(fptr);
	
	return 0;
}