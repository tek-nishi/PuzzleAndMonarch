//
// Field描画用
//
$version$
$precision$

uniform sampler2DShadow uShadowMap;
uniform float uShadowIntensity;

in vec4 vColor;
in vec4 vShadowCoord;

float uShininess = 120;
uniform vec4 uLightPosition;

uniform vec4 u_color; 

in vec4 vPosition;
in vec3 vNormal;

out vec4 Color;


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

	Color = (vColor + spec_color) * u_color * Shadow;
}
