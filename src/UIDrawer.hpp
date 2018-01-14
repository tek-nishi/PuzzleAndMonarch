#pragma once

//
// UI描画色々
//

#include <cinder/Json.h>
#include <cinder/gl/Shader.h>
#include "Font.hpp"
#include "Shader.hpp"


namespace ngs { namespace UI {

struct Drawer
{
  Drawer(const ci::JsonTree& params)
    : font_(params.getValueForKey<std::string>("ui.font.name"))
  {
    font_.size(params.getValueForKey<float>("ui.font.size"));
    
    {
      auto name = params.getValueForKey<std::string>("ui.shader.font");
      font_shader_ = createShader(name, name);
    }
    {
      auto color = ci::gl::ShaderDef().color();
      color_shader_ = ci::gl::getStockShader(color);
    }
  }


  Font& getFont() {
    return font_;
  }

  const ci::gl::GlslProgRef& getFontShader() const
  {
    return font_shader_;
  }

  const ci::gl::GlslProgRef& getColorShader() const
  {
    return color_shader_;
  }


private:
  Font font_;

  ci::gl::GlslProgRef font_shader_;
  ci::gl::GlslProgRef color_shader_;

};

} }
