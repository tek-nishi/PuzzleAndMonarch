#pragma once

//
// Cinderの実装で微妙なのを修正したやつ
//

#include <cinder/gl/gl.h>


namespace ngs {

void drawStrokedRect( const ci::Rectf &rect, float lineWidth ) noexcept;


#if defined (NGS_GL_IMPLEMENTATION)

void drawStrokedRect( const ci::Rectf &rect, float lineWidth ) noexcept
{
	const float halfWidth = lineWidth / 2;
	GLfloat verts[32];
	verts[0] = rect.x1 + halfWidth;		verts[1] = rect.y2 - halfWidth;		// left bar
	verts[2] = rect.x1 + halfWidth;		verts[3] = rect.y1 + halfWidth;
	verts[4] = rect.x1 - halfWidth;		verts[5] = rect.y2 + halfWidth;
	verts[6] = rect.x1 - halfWidth;		verts[7] = rect.y1 - halfWidth;
	verts[8] = rect.x1 + halfWidth;		verts[9] = rect.y1 + halfWidth;
	verts[10] = rect.x2 - halfWidth;	verts[11] = rect.y1 + halfWidth;	// upper bar
	verts[12] = rect.x1 - halfWidth;	verts[13] = rect.y1 - halfWidth;
	verts[14] = rect.x2 + halfWidth;	verts[15] = rect.y1 - halfWidth;
	verts[16] = rect.x2 - halfWidth;	verts[17] = rect.y1 + halfWidth;	// right bar
	verts[18] = rect.x2 - halfWidth;	verts[19] = rect.y2 - halfWidth;
	verts[20] = rect.x2 + halfWidth;	verts[21] = rect.y1 - halfWidth;
	verts[22] = rect.x2 + halfWidth;	verts[23] = rect.y2 + halfWidth;
	verts[24] = rect.x2 - halfWidth;	verts[25] = rect.y2 - halfWidth;	// bottom bar
	verts[26] = rect.x1 + halfWidth;	verts[27] = rect.y2 - halfWidth;
	verts[28] = rect.x2 + halfWidth;	verts[29] = rect.y2 + halfWidth;
	verts[30] = rect.x1 - halfWidth;	verts[31] = rect.y2 + halfWidth;

	auto ctx = ci::gl::context();
	const ci::gl::GlslProg* curGlslProg = ctx->getGlslProg();
	if( ! curGlslProg )
  {
		DOUT << "No GLSL program bound" << std::endl;
		return;
	}

	ctx->pushVao();
	ctx->getDefaultVao()->replacementBindBegin();

	ci::gl::VboRef defaultVbo = ctx->getDefaultArrayVbo( 32 * sizeof( float ) );
	ci::gl::ScopedBuffer bufferBindScp( defaultVbo );
	// defaultVbo->bufferSubData( 0, 32 * sizeof( float ), verts );
  defaultVbo->bufferData(32 * sizeof(float), verts, GL_STATIC_DRAW);

	int posLoc = curGlslProg->getAttribSemanticLocation( ci::geom::Attrib::POSITION );
	if( posLoc >= 0 )
  {
    ci::gl::enableVertexAttribArray( posLoc );
		ci::gl::vertexAttribPointer( posLoc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	}

	ctx->setDefaultShaderVars();
	ctx->getDefaultVao()->replacementBindEnd();	
	ctx->drawArrays( GL_TRIANGLE_STRIP, 0, 16 );
	ctx->popVao();
}

#endif

}
