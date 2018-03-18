//
// BG描画
// World座標とUVからCheckerboard模様を生成
//
$version$
$precision$

uniform sampler2D	uTex0;
uniform vec4 u_color;
uniform float u_checker_size;
uniform vec2 u_pos;

uniform vec4 u_bright;
uniform vec4 u_dark;
                                                          
in vec2 TexCoord0;

out vec4 oColor;


void main(void)
{
  vec2 pos = floor(u_pos + TexCoord0 * u_checker_size);
  float mask = mod(pos.x + mod(pos.y, 2.0), 2.0);
  oColor = texture(uTex0, TexCoord0) * u_color * mix(u_dark, u_bright, mask);
}
