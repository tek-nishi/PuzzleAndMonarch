//
// Effect描画用
//
$version$
$precision$

// NOTICE カメラのViewMatrixで変換
uniform vec4 uLightPosition;

uniform float uAmbient;
uniform float uShininess;
uniform vec4 uSpecular;

uniform vec4 u_color; 

in vec4 vColor;
in vec4 vPosition;
in vec3 vNormal;

out vec4 Color;


void main(void)
{
  // ライティング
  vec3 light   = normalize(uLightPosition.xyz * vPosition.w - vPosition.xyz * uLightPosition.w);
  vec3 view    = -normalize(vPosition.xyz);
  vec3 fnormal = normalize(vNormal);

  // 平行光源+影
  float diffuse = max(dot(light, fnormal), uAmbient);

  // スペキュラは反射ベクトルを求める方式
  vec3 reflect    = reflect(-light, fnormal);
  float specular  = pow(max(dot(reflect, view), 0.0), uShininess);
  vec4 spec_color = uSpecular * specular;

	Color = (vColor * diffuse + spec_color) * u_color;
}
