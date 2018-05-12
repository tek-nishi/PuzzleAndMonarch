//
// BG描画
// World座標とUVからCheckerboard模様を生成
//
$version$
$precision$

uniform sampler2DShadow uShadowMap;
uniform sampler2D	uTex1;

uniform float uShadowIntensity;

uniform vec4 u_color;
uniform float u_checker_size;
uniform vec2 u_pos;
uniform vec4 u_bright;
uniform vec4 u_dark;

float uShininess = 80;
uniform vec4 uLightPosition;

in vec4 vPosition;
in vec3 vNormal;

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

  vec3 light   = normalize((uLightPosition * vPosition.w - uLightPosition.w * vPosition).xyz);
  vec3 view    = -normalize(vPosition.xyz);
  vec3 fnormal = normalize(vNormal);

  // スペキュラは反射ベクトルを求める方式
  vec3 reflect   = reflect(-light, fnormal);
  float specular = pow(max(dot(reflect, view), 0.0), uShininess);
  vec4 spec_color = vec4(0.2, 0.2, 0.2, 1) * specular;

  vec2 pos = floor(u_pos + TexCoord0 * u_checker_size);
  float mask = mod(pos.x + mod(pos.y, 2.0), 2.0);
  oColor = (texture(uTex1, TexCoord0) * mix(u_dark, u_bright, mask) + spec_color) * u_color * Shadow;
}
