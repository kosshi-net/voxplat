#version 330

in vec2 texcoord;

uniform sampler2D uTex;
uniform vec4 uColor;

out vec4 out_Color;

void main(void) {
	out_Color = vec4(
		1.0f, 1.0f, 1.0f, 
		texture2D(uTex, texcoord).r
	) * uColor;
}
