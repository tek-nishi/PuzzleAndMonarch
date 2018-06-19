//
// Effect描画
//
$version$

uniform mat4 ciViewProjection;
uniform mat4 ciViewMatrix;
uniform mat3 ciNormalMatrix;

in mat4 vInstanceMatrix;

in vec4	ciPosition;
in vec3 ciNormal;

in vec3 uColor;

out vec4 vPosition;
out vec3 vNormal;
out vec3 vColor;


void main(void)
{
  vPosition = ciViewMatrix * vInstanceMatrix * ciPosition;
  vNormal   = ciNormalMatrix * ciNormal;
  vColor    = uColor;

	gl_Position = ciViewProjection * vInstanceMatrix * ciPosition;
}
