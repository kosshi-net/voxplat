#version 330

uniform vec4 uColor;

in float vDepth;
in vec4 vPos;

uniform float u_far;

out vec4 out_Color;

void main(void) {
	out_Color = uColor;
	gl_FragDepth = length(vPos)/u_far;
}
