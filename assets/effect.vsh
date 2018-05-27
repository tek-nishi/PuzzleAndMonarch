//
// Effect描画
//
$version$

uniform mat4 ciModelViewProjection;

in vec4	ciColor;
in vec4	ciPosition;

out vec4 vColor;


void main(void)
{
	vColor = ciColor;

	gl_Position = ciModelViewProjection * ciPosition;
}
