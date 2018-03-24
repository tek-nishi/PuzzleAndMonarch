//
// Field描画用
//
$version$
$precision$

uniform sampler2DShadow uShadowMap;
uniform float uShadowIntensity;

in vec4 vColor;
in vec4 vShadowCoord;

out vec4 Color;


void main(void)
{
	vec4 ShadowCoord = vShadowCoord / vShadowCoord.w;
	float Shadow = 1.0;
	
	// if ( ShadowCoord.z > -1 && ShadowCoord.z < 1 )
  {
		Shadow = mix(uShadowIntensity, 1.0, textureProj(uShadowMap, ShadowCoord, -0.0005));
	}

	Color = vColor * Shadow;
}
