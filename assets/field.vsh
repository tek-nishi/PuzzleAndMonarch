//
// Field描画
//
$version$

uniform mat4 ciModelViewProjection;
uniform vec4 u_color;

in vec4 ciPosition;
in vec4 ciColor;

out vec4 Color;


void main(void)
{
  gl_Position = ciModelViewProjection * ciPosition;
  Color       = ciColor * u_color;
}
