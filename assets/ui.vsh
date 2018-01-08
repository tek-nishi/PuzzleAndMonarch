//
// UI描画用
//
$version$

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec4 ciColor;

out vec4 Color;


void main(void) {
  vec4 position = ciModelViewProjection * ciPosition;

  gl_Position = position;
  Color       = ciColor;
}
