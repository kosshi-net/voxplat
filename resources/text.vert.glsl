#version 330

in vec4 aCoord;
out vec2 texcoord;

void main(void) {
  gl_Position = vec4(aCoord.xy, 0.0f, 1.0f);
  texcoord = aCoord.zw;
}
