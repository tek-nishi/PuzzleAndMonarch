#pragma once

//
// 角丸矩形描画Widget
//

#include "UIWidgetBase.hpp"
#include "UIDrawer.hpp"


namespace ngs { namespace UI {

class RoundRect
  : public UI::WidgetBase
{
  float corner_radius_;
  int corner_segment_ = 0;
  ci::ColorA color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
  bool fill_ = false;

  
public:
  RoundRect(const ci::JsonTree& params)
    : corner_radius_(params.getValueForKey<float>("corner_radius"))
  {
    if (params.hasChild("corner_segment"))
    {
      corner_segment_ = params.getValueForKey<int>("corner_segment");
    }
    if (params.hasChild("color"))
    {
      color_ = Json::getColorA<float>(params["color"]);
    }
    if (params.hasChild("fill"))
    {
      fill_ = params.getValueForKey<bool>("fill");
    }
  }

  ~RoundRect() = default;


private:
  void draw(const ci::Rectf& rect, UI::Drawer& drawer) noexcept override
  {
    ci::gl::ScopedGlslProg prog(drawer.getColorShader());
    ci::gl::color(color_);

    if (fill_)
    {
      ci::gl::drawSolidRoundedRect(rect, corner_radius_, corner_segment_);
    }
    else
    {
      ci::gl::drawStrokedRoundedRect(rect, corner_radius_, corner_segment_);
    }
  }

};

} }
