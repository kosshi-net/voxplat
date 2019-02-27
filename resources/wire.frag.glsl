#version 120

uniform vec4 uColor;

varying float vDepth;
varying vec4 vPos;

uniform float u_far;

void main(void) {
	gl_FragColor = uColor;
	gl_FragDepth = length(vPos)/u_far;
}
