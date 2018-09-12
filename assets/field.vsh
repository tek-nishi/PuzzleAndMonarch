//
// Field描画
//
$version$

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelMatrix;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

uniform mat4 uShadowMatrix;

uniform float uTopY = 1.0;

in vec3	ciColor;
in vec4	ciPosition;
in vec3 ciNormal;

out vec4 vPosition;
out vec3 vNormal;

out vec3 vColor;
out vec4 vShadowCoord;

const mat4 biasMatrix = mat4( 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0,
                              0.5, 0.5, 0.5, 1.0 );


void main(void)
{
  vec4 p = ciPosition;

  // Yが2以上の頂点のみスケーリングする
  float s = step(2.0, p.y);
  p.y = mix(p.y, (p.y - 2.0) * uTopY + 2.0, s);

  vShadowCoord = (biasMatrix * uShadowMatrix * ciModelMatrix) * p;
	vColor			 = ciColor;

  vPosition = ciModelView * p;
  vNormal   = ciNormalMatrix * ciNormal;

	gl_Position	 = ciModelViewProjection * p;
}
