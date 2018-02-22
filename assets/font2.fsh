//
// フォント描画用
// Signed distance field
// SOURCE https://github.com/libgdx/libgdx/wiki/Distance-field-fonts
//
$version$
$precision$

uniform sampler2D	uTex0;

uniform float u_buffer;
uniform float u_gamma;

in vec4 Color;
in vec2 TexCoord0;

out vec4 oColor;


void main(void)
{
  float dist  = texture(uTex0, TexCoord0).r;
  float alpha = smoothstep(u_buffer - u_gamma, u_buffer + u_gamma, dist);
  oColor = vec4(Color.rgb, Color.a * alpha);
}
