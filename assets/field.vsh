//
// Field描画
//
$version$

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelMatrix;

uniform mat4 uShadowMatrix;
uniform vec4 u_color; 

in vec4	ciColor;
in vec4	ciPosition;

out vec4 vColor;
out vec4 vShadowCoord;

const mat4 biasMatrix = mat4( 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0,
                              0.5, 0.5, 0.5, 1.0 );


void main(void)
{
	vColor			 = ciColor * u_color;
  vShadowCoord = (biasMatrix * uShadowMatrix * ciModelMatrix) * ciPosition;
	gl_Position	 = ciModelViewProjection * ciPosition;
}
