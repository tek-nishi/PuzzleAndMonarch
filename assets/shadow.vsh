//
// Field描画(Shadow buffer)
//
$version$

uniform mat4 ciModelViewProjection;

uniform float uTopY;

in vec4 ciPosition;


void main(void)
{
  vec4 p = ciPosition;

  // Yが2以上の頂点のみスケーリングする
  float s = step(2.0, p.y);
  p.y = mix(p.y, (p.y - 2.0) * uTopY + 2.0, s);
  
  gl_Position = ciModelViewProjection * p;
}
