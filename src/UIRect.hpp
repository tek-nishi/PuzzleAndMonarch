#pragma once

//
// 矩形描画Widget
//

#include "UIWidgetBase.hpp"
#include "UIDrawer.hpp"
#include "gl.hpp"


namespace ngs { namespace UI {

class Rect
  : public UI::WidgetBase
{
  ci::Color color_ = { 1.0f, 1.0f, 1.0f };
  bool fill_ = false;
  float line_width_ = 0.5;

  
public:
  Rect(const ci::JsonTree& params) noexcept
    : color_(Json::getColor<float>(params["color"])) 
  {
    if (params.hasChild("fill"))
    {
      fill_ = params.getValueForKey<bool>("fill");
    }
    if (params.hasChild("line_width"))
    {
      line_width_ = params.getValueForKey<float>("line_width");
    }
  }

  ~Rect() = default;


private:
  void draw(const ci::Rectf& rect, UI::Drawer& drawer, float alpha) noexcept override
  {
    ci::gl::ScopedGlslProg prog(drawer.getColorShader());
    ci::gl::color(ci::ColorA(color_, alpha));

    if (fill_)
    {
      ci::gl::drawSolidRect(rect);
    }
    else
    {
      drawStrokedRect(rect, line_width_);
    }
  }


  void setParam(const std::string& name, const boost::any& value) noexcept override
  {
    std::map<std::string, std::function<void (const boost::any& v)>> tbl = {
      { "fill", [this](const boost::any& v) noexcept
                {
                  fill_ = boost::any_cast<const bool>(v);
                }
      },
      { "color", [this](const boost::any& v) noexcept
                 {
                   color_ = boost::any_cast<const ci::Color&>(v);
                 }
      }
    };

    tbl.at(name)(value);
  }
  
  boost::any getParam(const std::string& name) noexcept override
  {
    std::map<std::string, std::function<boost::any ()>> tbl = {
      { "fill", [this]() noexcept
                {
                  return &fill_;
                }
      },
      { "color", [this]() noexcept
                 {
                   return &color_;
                 }
      }
    };

    return tbl.at(name)();
  }

};

} }
