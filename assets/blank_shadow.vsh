//
// Field描画(Shadow buffer)
//
$version$

uniform mat4 ciViewProjection;

in mat4 vInstanceMatrix;

in vec4 ciPosition;


void main(void)
{
	gl_Position	 = ciViewProjection * vInstanceMatrix * ciPosition;
}
