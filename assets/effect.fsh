//
// Effect描画用
//
$version$
$precision$

// NOTICE カメラのViewMatrixで変換
uniform vec4 uLightPosition;

uniform float uAmbient;

uniform vec3 u_color; 

in vec4 vPosition;
in vec3 vNormal;
in vec3 vColor; 

out vec4 Color;


void main(void)
{
  // ライティング
  vec3 light   = normalize(uLightPosition.xyz * vPosition.w - vPosition.xyz * uLightPosition.w);
  vec3 view    = -normalize(vPosition.xyz);
  vec3 fnormal = normalize(vNormal);

  // 平行光源
  float diffuse = max(dot(light, fnormal), uAmbient);
	Color = vec4((vColor * diffuse) * u_color, 1.0);
}
