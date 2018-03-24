//
// Field描画用
//
$version$
$precision$

uniform sampler2DShadow uShadowMap;

in vec4 vColor;
in vec4 vShadowCoord;

out vec4 Color;


void main(void)
{
	vec4 ShadowCoord	= vShadowCoord / vShadowCoord.w;
	float Shadow = 1.0;
	
	// if ( ShadowCoord.z > -1 && ShadowCoord.z < 1 )
  {
		Shadow = max(textureProj(uShadowMap, ShadowCoord, -0.00005), 0.8);
	}

	Color = vColor * Shadow;
}
