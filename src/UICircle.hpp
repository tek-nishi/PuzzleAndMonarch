#pragma once

//
// UI円描画
//


#include "UIWidgetBase.hpp"
#include "UIDrawer.hpp"


namespace ngs { namespace UI {

class Circle
  : public UI::WidgetBase
{
  float radius_;
  int segment_ = 0;
  ci::ColorA color_ = { 1.0f, 1.0f, 1.0f, 1.0f };
  bool fill_ = false;
  glm::vec2 alignment_ = { 0.5, 0.5 };

  
public:
  Circle(const ci::JsonTree& params)
    : radius_(params.getValueForKey<float>("radius"))
  {
    if (params.hasChild("segment"))
    {
      segment_ = params.getValueForKey<int>("segment");
    }
    if (params.hasChild("color"))
    {
      color_ = Json::getColorA<float>(params["color"]);
    }
    if (params.hasChild("fill"))
    {
      fill_ = params.getValueForKey<bool>("fill");
    }
    if (params.hasChild("alignment"))
    {
      alignment_ = Json::getVec<glm::vec2>(params["alignment"]);
    }
  }

  ~Circle() = default;


private:
  void draw(const ci::Rectf& rect, UI::Drawer& drawer) noexcept override
  {
    ci::gl::ScopedGlslProg prog(drawer.getColorShader());
    ci::gl::color(color_);

    auto center = rect.getCenter(); 

    if (fill_)
    {
      ci::gl::drawSolidCircle(center, radius_, segment_);
    }
    else
    {
      ci::gl::drawStrokedCircle(center, radius_, segment_);
    }
  }

};

} }
