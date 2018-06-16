#pragma once

//
// Cinderの実装で微妙なのを修正したやつ
//

#include <cinder/gl/gl.h>


namespace ngs {

void drawStrokedRect( const ci::Rectf &rect, float lineWidth ) noexcept;

//
// ci::Ring改変
//
class Ring
  : public ci::geom::Source
{
public:
  Ring();

  Ring&		center( const glm::vec2 &center ) { mCenter = center; return *this; }
  Ring&		radius( float radius );
  Ring&		width( float width );
  Ring&		subdivisions( int subdivs );
  Ring&   angle(float begin, float end);

  size_t		getNumVertices() const override;
  size_t		getNumIndices() const override { return 0; }
  ci::geom::Primitive	getPrimitive() const override { return ci::geom::Primitive::TRIANGLE_STRIP; }
  uint8_t		getAttribDims( ci::geom::Attrib attr ) const override;
  ci::geom::AttribSet	getAvailableAttribs() const override;
  void		loadInto( ci::geom::Target *target, const ci::geom::AttribSet &requestedAttribs ) const override;
  Ring*		clone() const override { return new Ring( *this ); }

private:
  void	updateVertexCounts();

  glm::vec2		mCenter;
  float		mRadius;
  float		mWidth;
  int			mRequestedSubdivisions, mNumSubdivisions;
  size_t		mNumVertices;
  
  float mBeginAngle;
  float mEndAngle;
};

void drawStrokedCircle( const glm::vec2 &center, float radius, float lineWidth, int numSegments );
void drawStrokedCircle( const glm::vec2 &center, float radius, float lineWidth, int numSegments,
                        float begin_angle, float end_angle );


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


//
// ci::Ring改変
//
Ring::Ring()
  : mRequestedSubdivisions( -1 ),
    mCenter( 0, 0 ),
    mRadius( 1.0f ),
    mWidth( 0.5f ),
    mBeginAngle(0.0f),
    mEndAngle(M_PI * 2.0f)
{
  updateVertexCounts();
}

Ring&	Ring::subdivisions( int subdivs )
{
  mRequestedSubdivisions = subdivs;
  updateVertexCounts();
  return *this;
}

Ring&	Ring::radius( float radius )
{
  mRadius = radius;
  updateVertexCounts();
  return *this;
}

Ring&	Ring::width( float width )
{
  mWidth = width;
  return *this;
}

Ring& Ring::angle(float begin, float end)
{
  mBeginAngle = ci::toRadians(begin);
  mEndAngle   = ci::toRadians(end);
  return *this;
}

// If numSegments<0, calculate based on radius
void Ring::updateVertexCounts()
{
  if( mRequestedSubdivisions <= 0 )
  {
    mNumSubdivisions = (int) ci::math<double>::floor( mRadius * float( M_PI * 2 ) );
  }
  else
  {
    mNumSubdivisions = mRequestedSubdivisions;
  }

  if( mNumSubdivisions < 3 ) mNumSubdivisions = 3;
  mNumVertices = ( mNumSubdivisions + 1 ) * 2;
}

size_t Ring::getNumVertices() const
{
  return mNumVertices;
}

uint8_t	Ring::getAttribDims( ci::geom::Attrib attr ) const
{
  switch( attr )
  {
  case ci::geom::Attrib::POSITION: return 2;
  default:
    return 0;
  }
}

ci::geom::AttribSet Ring::getAvailableAttribs() const
{
  return { ci::geom::Attrib::POSITION };
}

void Ring::loadInto( ci::geom::Target *target, const ci::geom::AttribSet &/*requestedAttribs*/ ) const
{
  std::vector<glm::vec2> positions;
  positions.reserve( mNumVertices );

  float innerRadius = mRadius - 0.5f * mWidth;
  float outerRadius = mRadius + 0.5f * mWidth;

  // iterate the segments
  const float tDelta = 1 / (float) mNumSubdivisions * (mEndAngle - mBeginAngle);
  float t = mBeginAngle;
  for( int s = 0; s <= mNumSubdivisions; s++ )
  {
    glm::vec2 unit( ci::math<float>::cos( t ), ci::math<float>::sin( t ) );
    positions.emplace_back( mCenter + unit * innerRadius );
    positions.emplace_back( mCenter + unit * outerRadius );
    t += tDelta;
  }

  target->copyAttrib( ci::geom::Attrib::POSITION, 2, 0, (const float*) positions.data(), mNumVertices );
}

void drawStrokedCircle( const glm::vec2 &center, float radius, float lineWidth, int numSegments )
{
	if( numSegments <= 0 )
  {
		numSegments = (int)ci::math<double>::floor( radius * M_PI * 2 );
  }

  ci::gl::draw( Ring().center( center ).radius( radius ).width( lineWidth ).subdivisions( numSegments ) );
}

void drawStrokedCircle( const glm::vec2 &center, float radius, float lineWidth, int numSegments,
                        float begin_angle, float end_angle )
{
	if( numSegments <= 0 )
  {
		numSegments = (int)ci::math<double>::floor( radius * M_PI * 2 );
  }

  ci::gl::draw( Ring().center( center ).radius( radius ).width( lineWidth ).angle(begin_angle, end_angle).subdivisions( numSegments ) );
}


#endif

}
