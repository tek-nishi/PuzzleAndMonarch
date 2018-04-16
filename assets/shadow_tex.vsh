//
// Field描画(Shadow buffer)
//
$version$

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;

out vec2 TexCoord0;


void main(void)
{
  TexCoord0   = ciTexCoord0;
  gl_Position = ciModelViewProjection * ciPosition;
}
