//
// BG描画
// World座標とUVからCheckerboard模様を生成
//
$version$
$precision$

uniform sampler2DShadow uShadowMap;
uniform sampler2D	uTex1;

uniform vec4 u_color;
uniform float u_checker_size;
uniform vec2 u_pos;
uniform vec4 u_bright;
uniform vec4 u_dark;

// NOTICE カメラのViewMatrixで変換
uniform vec4 uLightPosition;

uniform float uAmbient;
uniform float uShininess;
uniform vec4 uSpecular;

in vec4 vPosition;
in vec3 vNormal;

in vec2 TexCoord0;
in vec4 vShadowCoord;

out vec4 oColor;


void main(void)
{
	vec4 ShadowCoord = vShadowCoord / vShadowCoord.w;
	float shadow = 1.0;
	
	// if ( ShadowCoord.z > -1 && ShadowCoord.z < 1 )
  {
		shadow = mix(0.0, 1.0, textureProj(uShadowMap, ShadowCoord, -0.0005));
	}

  // ライティング
  vec3 light   = normalize(uLightPosition.xyz * vPosition.w - vPosition.xyz * uLightPosition.w);
  vec3 view    = -normalize(vPosition.xyz);
  vec3 fnormal = normalize(vNormal);

  // 平行光源
  float diffuse = max(dot(light, fnormal) * shadow, uAmbient);

  // スペキュラは反射ベクトルを求める方式
  vec3 reflect    = reflect(-light, fnormal);
  float specular  = pow(max(dot(reflect, view), 0.0), uShininess);
  vec4 spec_color = uSpecular * specular;

  vec2 pos = floor(u_pos + TexCoord0 * u_checker_size);
  float mask = mod(pos.x + mod(pos.y, 2.0), 2.0);
  oColor = texture(uTex1, TexCoord0) * (mix(u_dark, u_bright, mask) * diffuse + spec_color) * u_color;
}
