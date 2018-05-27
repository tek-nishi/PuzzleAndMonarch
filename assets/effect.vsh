//
// Effect描画
//
$version$

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

in vec4	ciColor;
in vec4	ciPosition;
in vec3 ciNormal;

out vec4 vPosition;
out vec3 vNormal;

out vec4 vColor;


void main(void)
{
	vColor = ciColor;

  vPosition = ciModelView * ciPosition;
  vNormal   = ciNormalMatrix * ciNormal;

	gl_Position = ciModelViewProjection * ciPosition;
}
