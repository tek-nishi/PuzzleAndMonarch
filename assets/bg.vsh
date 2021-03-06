//
// BG描画
//
$version$

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelMatrix;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;

uniform mat4 uShadowMatrix;

in vec4 ciPosition;
in vec2 ciTexCoord0;
in vec3 ciNormal;

out vec4 vPosition;
out vec3 vNormal;

out vec2 TexCoord0;
out vec4 vShadowCoord;

const mat4 biasMatrix = mat4( 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0,
                              0.5, 0.5, 0.5, 1.0 );

void main(void)
{
	vShadowCoord = (biasMatrix * uShadowMatrix * ciModelMatrix) * ciPosition;
  TexCoord0    = ciTexCoord0;

  vPosition = ciModelView * ciPosition;
  vNormal   = ciNormalMatrix * ciNormal;

  gl_Position = ciModelViewProjection * ciPosition;
}
