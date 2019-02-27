#ifndef GFX_TEXT_H
#define GFX_TEXT_H 1
	
#define FONT_SIZE 16

int gfx_text_init(void);

// String, X, Y, scaleX, scaleY, vec4 color, font_size
int gfx_text_draw(const char*, float, float, float, float, float*, int);

#endif

