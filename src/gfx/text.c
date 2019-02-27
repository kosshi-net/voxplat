#define __FILENAME__ "gfx/text.c"

#include <config.h>

#include "event.h"
#include "text.h"
#include "gfx.h"
#include "ppm.h"
#include "res.h"
#include "mem.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <glad/glad.h>

#include <stdio.h>

FT_Library ft;
FT_Face face;
FT_GlyphSlot g;

GLuint text_vao;

GLuint text_shader;
GLuint text_shader_a_coord;
GLuint text_shader_u_tex;
GLuint text_shader_u_color;

GLuint text_vbo;
GLuint text_tex;


void gfx_gen_atlas(void);

int gfx_text_init()
{
	logf_info("Init");
	if (FT_Init_FreeType(&ft)){
		log_error("Freetype failed");
		return 1;
	}

	if( FT_New_Memory_Face( ft, 
		res_file(font_ttf), 
		res_size(font_ttf), 
		0, &face  )  
	) {
		log_error( "Could not open font!" );
		return 1;	
	}
	
	FT_Set_Pixel_Sizes(face, 0, FONT_SIZE);
	
	if (FT_Load_Char( face, 'X', FT_LOAD_RENDER )) {
		log_error("Freetype can't load char!");
		return 1;
	}


	// OPENGL
	glGenVertexArrays(1, &text_vao);
	glBindVertexArray(text_vao);

	char* text_vert_src = res_strcpy( text_vert_glsl  );
	char* text_frag_src = res_strcpy( text_frag_glsl  );
	text_shader = gfx_create_shader( text_vert_src, text_frag_src );
	mem_free( text_vert_src );	
	mem_free( text_frag_src );

	glUseProgram(text_shader);
	text_shader_a_coord = 	glGetAttribLocation (text_shader, "aCoord");
	text_shader_u_tex = 	glGetUniformLocation(text_shader, "uTex");
	text_shader_u_color = 	glGetUniformLocation(text_shader, "uColor");


	glGenBuffers( 1, &text_vbo );
	glEnableVertexAttribArray( text_shader_a_coord );
	glBindBuffer( GL_ARRAY_BUFFER, text_vbo );
	glVertexAttribPointer( text_shader_a_coord, 4, GL_FLOAT, GL_FALSE, 0, NULL );

	// Finish up

	gfx_gen_atlas();

	logf_info("Init ok");

	return 0;
}


struct {

	int width;
	int height;

	int x;
	int y;

	int left;
	int top;

	int advance;

} AChar[128];

int atlasHeight = 0;
int atlasWidth = 0;

unsigned char*atlas;

float* geometry_buffer;



void gfx_gen_atlas(){
	g = face->glyph;

	int total_x = 0;
	int max_y = 0;
	for (int i = 0; i < 128; ++i)
		{
		if(FT_Load_Char(face, i, FT_LOAD_RENDER))
			continue;

		AChar[i].width  = g->bitmap.width;
		AChar[i].height = g->bitmap.rows;

		AChar[i].left = g->bitmap_left;
		AChar[i].top = g->bitmap_top;

		AChar[i].advance = g->advance.x/64;

		total_x += AChar[i].advance;
		max_y = (AChar[i].height > max_y) ? AChar[i].height : max_y;

		AChar[i].x = total_x;
		AChar[i].y = 0;
	}



	atlasHeight = max_y;
	atlasWidth  = total_x;

	atlas = (unsigned char*) mem_alloc(atlasHeight*atlasWidth);

	for (int i = 0; i < 128; ++i)
	{
		if(FT_Load_Char(face, i, FT_LOAD_RENDER))
			continue;

		for (int y = 0; y < AChar[i].height; ++y)
		for (int x = 0; x < AChar[i].width;  ++x)
		{
			atlas[ atlasWidth * y + AChar[i].x + x ] =
				face->glyph->bitmap.buffer[ AChar[i].width * y + x];
		}
	}

	#if DEBUG_GFX_TEXT_WRITE_PPM
	logf_info( "DEBUG_GFX_TEXT_WRITE_PPM %i", DEBUG_GFX_TEXT_WRITE_PPM  );
	gfx_util_write_ppm(atlasWidth, atlasHeight, atlas);
	#endif
	
	geometry_buffer = (float*)malloc( 1024 * sizeof(float)*4*4 );

	glUseProgram(text_shader);
	glBindVertexArray(text_vao);
	
	glActiveTexture(	GL_TEXTURE0 );
	glGenTextures(		1, &text_tex );
	glBindTexture(		GL_TEXTURE_2D, text_tex );
	//glUniform1i(		text_shader_u_tex, text_tex );
	
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	glTexImage2D(
		GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, 
		GL_RED, GL_UNSIGNED_BYTE, atlas
	);


	glBindBuffer( 	GL_ARRAY_BUFFER,0 );
	glBindTexture(	GL_TEXTURE_2D, 	0 );

//	mem_free( atlas );
}

int gfx_text_draw(
	const char *text, 
	const float _x, const float _y, 
	const float sx, const float sy,
	float color[4], int font_size
	// s = scale
){
	float x = _x;
	float y = _y;
	const char *p;

	int bufIndex = 0;
	int count = 0;

	int newlines = 0;

	for(p = text; *p; p++) {
		int id = *p;
		
		if ( id=='\n' ){
			x = _x;
			y = y - font_size*sy; 
			newlines++;
			continue;
		}

		float x2 = x + AChar[id].left * sx;
		float y2 = -y - AChar[id].top * sy;
		float w = AChar[id].width * sx;
		float h = AChar[id].height * sy;

		float uv[4] = {
			AChar[id].x / (float)atlasWidth,
			AChar[id].y / (float)atlasHeight,
			(AChar[id].x + AChar[id].width)	 / (float)atlasWidth,
			(AChar[id].y + AChar[id].height) / (float)atlasHeight,

		};

		GLfloat box[4*6] = {
				x2,     -y2    , uv[0], uv[1],
				x2 + w, -y2    , uv[2], uv[1],
				x2,     -y2 - h, uv[0], uv[3],

				x2 + w, -y2    , uv[2], uv[1],
				x2,     -y2 - h, uv[0], uv[3],
				x2 + w, -y2 - h, uv[2], uv[3],
		};

		for (int i = 0; i < 4*6; ++i) {
			geometry_buffer[bufIndex++] = box[i];
		}
		count++;
		
		x += (AChar[id].advance) * sx;
	}

	
	glUseProgram(text_shader);
	glBindVertexArray(text_vao);

	glUniform4fv(text_shader_u_color, 1, color);

	glActiveTexture(	GL_TEXTURE0 );
	glBindTexture(		GL_TEXTURE_2D, text_tex );
	//glUniform1i(		text_shader_u_tex, 0 );

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_ARRAY_BUFFER, text_vbo);
	glBufferData(
		GL_ARRAY_BUFFER, 
		bufIndex*sizeof(float), 
		geometry_buffer, 
		GL_DYNAMIC_DRAW
	);

	glDrawArrays(GL_TRIANGLES, 0, 6*count);

	glDisable(GL_BLEND);

	return newlines;
}

