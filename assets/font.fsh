//
// フォント描画用
//
$version$
$precision$

uniform sampler2D	uTex0;
                                                          
in vec4 Color;
in vec2 TexCoord0;

out vec4 oColor;


void main(void) {
  oColor = vec4(1, 1, 1, texture(uTex0, TexCoord0).r) * Color;
}
