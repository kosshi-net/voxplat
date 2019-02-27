#version 120

varying vec2 texcoord;
uniform sampler2D uTex;
uniform vec4 uColor;

void main(void) {
	gl_FragColor = vec4(
		1.0f, 1.0f, 1.0f, 
		texture2D(uTex, texcoord).r
	) * uColor;
}
