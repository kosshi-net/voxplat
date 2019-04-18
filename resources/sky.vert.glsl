#version 330

in vec2 aCoord;

void main(void) {
	gl_Position = vec4( aCoord.xy,0.0,1.0);
} 