#define __FILENAME__ "import.c"
#include "import.h"
#include "chunk.h"
#include "worldedit.h"
#include "res.h"
#include <assert.h>


void load_map(const uint8_t *v, int len, struct ChunkSet *set);

void import_vxl_map(struct ChunkSet *set){


	load_map( res_file( map_vxl ), res_size( map_vxl  ), set );


}



/*
 * this code is adapted from the sample code in vxlform.txt
 */

void setgeom(int x, int y, int z, int t, struct ChunkSet *set)
{
//	assert(z >= 0 && z < 64);
	//map[x][y][z] = t;
	worldedit_write( set, (uint32_t[]){x,64-z,y} , t);
	
}

// need to convert for endianness here if we read 32-bits at a time
void setcolor(int x, int y, int z, uint32_t c, struct ChunkSet *set)
{


	worldedit_write( set, (uint32_t[]){x,64-z,y} , c);
//   assert(z >= 0 && z < 64);
//	color[x][y][z] = c;
}

void load_map( const uint8_t *v, int len, struct ChunkSet *set)
{
   const uint8_t *base = v;
   int x,y,z;
   for (y=0; y < 512; ++y) {
      for (x=0; x < 512; ++x) {
         for (z=0; z < 64; ++z) {
            setgeom(x,y,z,1, set);
         }
         z = 0;
         for(;;) {
            uint32_t *color;
            int i;
            int number_4byte_chunks = v[0];
            int top_color_start = v[1];
            int top_color_end   = v[2]; // inclusive
            int bottom_color_start;
            int bottom_color_end; // exclusive
            int len_top;
            int len_bottom;

            for(i=z; i < top_color_start; i++)
               setgeom(x,y,i,0, set);

            color = (uint32_t *) (v+4);
            for(z=top_color_start; z <= top_color_end; z++)
               setcolor(x,y,z,*color++, set);

            len_bottom = top_color_end - top_color_start + 1;

            // check for end of data marker
            if (number_4byte_chunks == 0) {
               // infer ACTUAL number of 4-byte chunks from the length of the color data
               v += 4 * (len_bottom + 1);
               break;
            }

            // infer the number of bottom colors in next span from chunk length
            len_top = (number_4byte_chunks-1) - len_bottom;

            // now skip the v pointer past the data to the beginning of the next span
            v += v[0]*4;

            bottom_color_end   = v[3]; // aka air start
            bottom_color_start = bottom_color_end - len_top;

            for(z=bottom_color_start; z < bottom_color_end; ++z) {
               setcolor(x,y,z,*color++, set);
            }
         }
      }
   }
   assert(v-base == len);
}


