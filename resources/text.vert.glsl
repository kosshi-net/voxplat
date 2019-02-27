#version 120

attribute vec4 aCoord;
varying vec2 texcoord;

void main(void) {
  gl_Position = vec4(aCoord.xy, 0.0f, 1.0f);
  texcoord = aCoord.zw;
}
