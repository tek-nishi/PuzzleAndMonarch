//
// BG描画
// World座標とUVからCheckerboard模様を生成
//
$version$
$precision$

uniform sampler2DShadow uShadowMap;
uniform sampler2D	uTex;

uniform float uShadowIntensity;

uniform vec4 u_color;
uniform float u_checker_size;
uniform vec2 u_pos;
uniform vec4 u_bright;
uniform vec4 u_dark;
                                                          
in vec2 TexCoord0;
in vec4 vShadowCoord;

out vec4 oColor;


void main(void)
{
	vec4 ShadowCoord = vShadowCoord / vShadowCoord.w;
	float Shadow = 1.0;
	
	// if ( ShadowCoord.z > -1 && ShadowCoord.z < 1 )
  {
		Shadow = mix(uShadowIntensity, 1.0, textureProj(uShadowMap, ShadowCoord, -0.0005));
	}

  vec2 pos = floor(u_pos + TexCoord0 * u_checker_size);
  float mask = mod(pos.x + mod(pos.y, 2.0), 2.0);
  oColor = (texture(uTex, TexCoord0) * u_color * mix(u_dark, u_bright, mask)) * Shadow;
}
