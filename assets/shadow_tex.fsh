//
// Field描画(Shadow buffer)
//
$version$
$precision$

uniform vec4 uColor;
uniform float uThreshold;
uniform sampler2D	uTex0;
in vec2 TexCoord0;



out vec4 oColor;


void main(void)
{
  vec4 color = texture(uTex0, TexCoord0);
  if (color.a < uThreshold) discard;

  oColor = vec4(uColor);
}
