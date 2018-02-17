#pragma once

//
// フォントを扱う
//

#include <boost/noncopyable.hpp>
#include <cassert>
#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/TriMesh.h>

#if defined (NGS_FONT_IMPLEMENTATION)
#define FONS_VERTEX_COUNT 2048
#define FONTSTASH_IMPLEMENTATION
#endif
#include "fontstash.h"


namespace ngs {

class Font
  : private boost::noncopyable
{
  struct Context
  {
    ci::gl::Texture2dRef tex;
    int width, height;
  };

  Context gl_;
  FONScontext* context_;

  float font_size_;


  // 以下、fontstashからのコールバック関数
  static int create(void* userPtr, int width, int height) noexcept;
  static int resize(void* userPtr, int width, int height) noexcept;
  static void update(void* userPtr, int* rect, const unsigned char* data) noexcept;
  static void draw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) noexcept;


public:
  Font(const std::string& path,
       int texture_width = 1024, int texture_height = 1024, float initial_size = 16) noexcept;

  ~Font();


  // フォントサイズ指定
  void size(float size) noexcept;
  float getSize() const noexcept;

  // 描画した時のサイズを取得
  ci::vec2 drawSize(const std::string& text) noexcept;

  // 描画
  // text  表示文字列
  // pos   表示位置
  // color 表示色
  void draw(const std::string& text, const glm::vec2& pos, const ci::ColorA& color) noexcept;
};


#if defined (NGS_FONT_IMPLEMENTATION)

int Font::create(void* userPtr, int width, int height) noexcept
{
  DOUT << "Font::create: " << width << "," << height << std::endl;

  Context* gl = (Context*)userPtr;

  // TIPS:テクスチャ内部形式をGL_R8にしといて
  //      シェーダーでなんとかする方式(from nanoVG)
  gl->tex = ci::gl::Texture2d::create(width, height,
                                      ci::gl::Texture2d::Format().dataType(GL_UNSIGNED_BYTE).internalFormat(GL_R8));

  gl->width  = width;
  gl->height = height;

  return 1;
}

int Font::resize(void* userPtr, int width, int height) noexcept
{
  // Reuse create to resize too.
  return create(userPtr, width, height);
}

void Font::update(void* userPtr, int* rect, const unsigned char* data) noexcept
{
  DOUT << "Font::update" << std::endl;

  Context* gl = (Context*)userPtr;
  if (!gl->tex.get()) return;

  int w = rect[2] - rect[0];
  int h = rect[3] - rect[1];

  // TIPS:data側も切り抜いて転送するので
  //      その指定も忘れない
  // glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, gl->width);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, rect[0]);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, rect[1]);

  gl->tex->update(data, GL_RED, GL_UNSIGNED_BYTE, 0, w, h, ci::ivec2(rect[0], rect[1]));
}

// FIXME: ci::glのコードを参考にした
//        ちょいと重い
void Font::draw(void* userPtr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts) noexcept
{
  using namespace ci;

  Context* gl = (Context*)userPtr;
  if (!gl->tex.get()) return;

  auto* ctx = gl::context();
  const gl::GlslProg* curGlslProg = ctx->getGlslProg();

  size_t vtx_size = sizeof(float) * nverts * 2;
  size_t total_array_size = vtx_size * 2 + nverts * 4;
  
  // データをまとめる
  std::vector<char> data(total_array_size);
  std::memcpy(&data[0], verts, vtx_size);
  std::memcpy(&data[vtx_size], tcoords, vtx_size);
  std::memcpy(&data[vtx_size * 2], colors, nverts * 4);

  ctx->pushVao();
  ctx->getDefaultVao()->replacementBindBegin();

  gl::VboRef defaultArrayVbo = ctx->getDefaultArrayVbo(total_array_size);
  gl::ScopedBuffer vboScp(defaultArrayVbo);
  // まとめたデータを一気に転送
  defaultArrayVbo->bufferSubData(0, total_array_size, &data[0]);

  {
    int loc = curGlslProg->getAttribSemanticLocation(geom::Attrib::POSITION);
    gl::enableVertexAttribArray(loc);
    gl::vertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
  }

  {
    int loc = curGlslProg->getAttribSemanticLocation(geom::Attrib::TEX_COORD_0);
    gl::enableVertexAttribArray(loc);
    gl::vertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, (void*)(vtx_size));
  }

  {
    int loc = curGlslProg->getAttribSemanticLocation(geom::Attrib::COLOR);
    gl::enableVertexAttribArray(loc);
    gl::vertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, (void*)(vtx_size * 2));
  }

  ctx->getDefaultVao()->replacementBindEnd();

  gl::ScopedTextureBind texScp(gl->tex);
  ctx->setDefaultShaderVars();
  ctx->drawArrays(GL_TRIANGLES, 0, nverts);
  ctx->popVao();
}


Font::Font(const std::string& path,
           int texture_width, int texture_height, float initial_size) noexcept
{
  auto full_path = getAssetPath(path).string();

  FONSparams params;

  memset(&params, 0, sizeof(params));
  params.width  = texture_width;
  params.height = texture_height;
  params.flags  = (unsigned char)FONS_ZERO_BOTTOMLEFT;

  params.renderCreate = Font::create;
  params.renderResize = Font::resize;
  params.renderUpdate = Font::update;
  params.renderDraw   = Font::draw;
  params.renderDelete = nullptr;

  params.userPtr = &gl_;

  context_ = fonsCreateInternal(&params);
  assert(context_);

  fonsClearState(context_);
  int handle = fonsAddFont(context_, "font", full_path.c_str());
  fonsSetFont(context_, handle);
  // TIPS:下揃えにしておくと、下にはみ出す部分も正しく扱える
  fonsSetAlign(context_, FONS_ALIGN_BOTTOM);
  fonsSetSize(context_, initial_size);

  font_size_ = initial_size;

  DOUT << "Font(" << path << ") handle: " << handle << std::endl;
}

Font::~Font()
{
  fonsDeleteInternal(context_);
}

void Font::size(float size) noexcept
{
  fonsSetSize(context_, size);
  font_size_ = size;
}

float Font::getSize() const noexcept
{
  return font_size_;
}

ci::vec2 Font::drawSize(const std::string& text) noexcept
{
  float bounds[4];
  fonsTextBounds(context_, 0, 0, text.c_str(), nullptr, bounds);

  return ci::vec2{ bounds[2], bounds[3] };
}

void Font::draw(const std::string& text, const glm::vec2& pos, const ci::ColorA& color) noexcept
{
  unsigned char r8 = color.r * 255.0f;
  unsigned char g8 = color.g * 255.0f;
  unsigned char b8 = color.b * 255.0f;
  unsigned char a8 = color.a * 255.0f;
    
  unsigned int c = (r8) | (g8 << 8) | (b8 << 16) | (a8 << 24);
  fonsSetColor(context_, c);
  fonsDrawText(context_, pos.x, pos.y, text.c_str(), nullptr);
}


#endif

}
