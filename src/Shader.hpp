#pragma once

//
// シェーダー
//   GLSL3.30と3.0ESの違いを吸収する
//

#include <fstream>
#include <utility>
#include <regex>
#include <cassert>
#include <cinder/gl/GlslProg.h>
#include "Path.hpp"


namespace ngs {

using Shader = std::pair<std::string, std::string>;

ci::gl::GlslProgRef createShader(const std::string& vertex_path,
                                 const std::string& fragment_path) noexcept;


#if defined (NGS_SHADER_IMPLEMENTATION)

// テキストファイル -> std::string
std::string readFile(const std::string& path) noexcept
{
  std::ifstream ifs(path);
  assert(ifs);

  std::string str((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());

  return str;
}


// 機種依存部分を置換
std::string replaceText(std::string text) noexcept
{
  std::vector<std::pair<std::regex, std::string>> replace {
#if defined (CINDER_COCOA_TOUCH)
    { std::regex("\\$version\\$"), "#version 300 es" },
    { std::regex("\\$precision\\$"), "precision mediump float;" },
#else
    { std::regex("\\$version\\$"), "#version 330" },
    { std::regex("\\$precision\\$"), "" },
#endif
  };

  for (const auto& t : replace)
  {
    text = std::regex_replace(text, t.first, t.second);
  }

  return text;
}


// シェーダーを読み込む
//   パスに拡張子は要らない
Shader readShader(const std::string& vertex_path,
                  const std::string& fragment_path) noexcept
{
  auto vertex_shader = readFile(getAssetPath(vertex_path + ".vsh").string());
  vertex_shader      = replaceText(vertex_shader);
  
  auto fragment_shader = readFile(getAssetPath(fragment_path + ".fsh").string());
  fragment_shader      = replaceText(fragment_shader);

  return { vertex_shader, fragment_shader };
}

// 生成
ci::gl::GlslProgRef createShader(const std::string& vertex_path,
                                 const std::string& fragment_path) noexcept
{
  auto shader = readShader(vertex_path, fragment_path);
  return ci::gl::GlslProg::create(shader.first, shader.second);
}

#endif

}
