#pragma once

//
// UI描画色々
//

#include <cinder/Json.h>
#include <cinder/gl/Shader.h>
#include "Font.hpp"
#include "Shader.hpp"
#include <map>


namespace ngs { namespace UI {

struct Drawer
  : private boost::noncopyable
{
  Drawer(const ci::JsonTree& params) noexcept
  {
    int texture_size = params.getValueForKey<int>("font_texture");
    float blur = params.getValueForKey<float>("font_blur");
    const auto& json = params["font"];
    for (size_t i = 0; i < json.getNumChildren(); ++i)
    {
      const auto& p = json[i];

      const auto& name = p.getValueForKey<std::string>("name");
      const auto& path = p.getValueForKey<std::string>("path");
      int initial_size = p.getValueForKey<int>("size");

      fonts_.emplace(std::piecewise_construct,
                     std::forward_as_tuple(name),
                     std::forward_as_tuple(path, texture_size, texture_size, initial_size));

      // Signed fidle distance用のブラー値
      fonts_.at(name).setBlur(blur);
    }

    {
      auto p = params["shader.font"];
      auto name = p.getValueForKey<std::string>("name");
      font_shader_ = createShader(name, name);
      font_shader_->uniform("u_buffer", p.getValueForKey<float>("u_buffer"));
      font_shader_->uniform("u_gamma",  p.getValueForKey<float>("u_gamma"));
    }
    {
      auto color = ci::gl::ShaderDef().color();
      color_shader_ = ci::gl::getStockShader(color);
    }
  }


  Font& getFont(const std::string& name) noexcept
  {
    return fonts_.at(name);
  }

  const ci::gl::GlslProgRef& getFontShader() const noexcept
  {
    return font_shader_;
  }

  const ci::gl::GlslProgRef& getColorShader() const noexcept
  {
    return color_shader_;
  }


#if defined (DEBUG)

  const std::map<std::string, Font>& getFonts() const noexcept
  {
    return fonts_;
  }

#endif


private:
  std::map<std::string, Font> fonts_;

  ci::gl::GlslProgRef font_shader_;
  ci::gl::GlslProgRef color_shader_;

};

} }
