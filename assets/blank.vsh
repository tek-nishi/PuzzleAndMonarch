//
// Blank描画
//
$version$

uniform mat4 ciViewProjection;
uniform mat4 ciViewMatrix;
uniform mat3 ciNormalMatrix;

uniform mat4 uShadowMatrix;

in vec3	ciColor;
in vec4	ciPosition;
in vec3 ciNormal;
in mat4 vInstanceMatrix;
in float uDiffusePower;

out vec4 vPosition;
out vec3 vNormal;

out vec3 vColor;
out vec4 vShadowCoord;
out float vDiffusePower;

const mat4 biasMatrix = mat4( 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0,
                              0.5, 0.5, 0.5, 1.0 );


void main(void)
{
  vShadowCoord = (biasMatrix * uShadowMatrix * vInstanceMatrix) * ciPosition;
	vColor			 = ciColor;

  vPosition = ciViewMatrix * vInstanceMatrix * ciPosition;
  vNormal   = ciNormalMatrix * ciNormal;

  vDiffusePower = uDiffusePower;

	gl_Position	 = ciViewProjection * vInstanceMatrix * ciPosition;
}
