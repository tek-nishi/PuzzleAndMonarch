//
// Effect描画
//
$version$

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

in vec4	ciPosition;
in vec3 ciNormal;

out vec4 vPosition;
out vec3 vNormal;


void main(void)
{
  vPosition = ciModelView * ciPosition;
  vNormal   = ciNormalMatrix * ciNormal;

	gl_Position = ciModelViewProjection * ciPosition;
}
