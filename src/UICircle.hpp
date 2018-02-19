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
  ci::Color color_ = { 1.0f, 1.0f, 1.0f };
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
      color_ = Json::getColor<float>(params["color"]);
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
  void draw(const ci::Rectf& rect, UI::Drawer& drawer, float alpha) noexcept override
  {
    ci::gl::ScopedGlslProg prog(drawer.getColorShader());
    ci::gl::color(ci::ColorA(color_, alpha));

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


  void setParam(const std::string& name, const boost::any& value) noexcept override
  {
    std::map<std::string, std::function<void (const boost::any& v)>> tbl = {
      { "radius", [this](const boost::any& v) noexcept
                  {
                    radius_ = boost::any_cast<const float>(v);
                  }
      },
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
      { "radius", [this]() noexcept
                  {
                    return &radius_;
                  }
      },
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
