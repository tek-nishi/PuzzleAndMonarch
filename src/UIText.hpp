#pragma once

//
// テキスト描画Widget
//

#include "UIWidgetBase.hpp"


namespace ngs { namespace UI {

class Text
  : public WidgetBase
{
  // レイアウト
  enum
  {
    LEFT   = 1 << 0,
    CENTER = 1 << 1,
    RIGHT  = 1 << 2,

    TOP    = 1 << 3,
    MIDDLE = 1 << 4,
    BOTTOM = 1 << 5,
  };
  
  std::string text_;

  std::string font_name_;
  float text_size_ = 1.0f;
  int layout_ = CENTER | MIDDLE;
  ci::ColorA color_ = { 1.0f, 1.0f, 1.0f, 1.0f };


public:
  Text(const ci::JsonTree& params) noexcept
    : text_(params.getValueForKey<std::string>("text")),
      font_name_(params.getValueForKey<std::string>("font"))
  {
    if (params.hasChild("text_size"))
    {
      text_size_ = params.getValueForKey<float>("text_size");
    }
    if (params.hasChild("layout"))
    {
      std::map<std::string, int> tbl = {
        { "left top",      LEFT | TOP },    
        { "left middle",   LEFT | MIDDLE },    
        { "left bottom",   LEFT | BOTTOM },    
        { "center top",    CENTER | TOP },    
        { "center middle", CENTER | MIDDLE },    
        { "center bottom", CENTER | BOTTOM },    
        { "right top",     RIGHT | TOP },    
        { "right middle",  RIGHT | MIDDLE },    
        { "right bottom",  RIGHT | BOTTOM },    
      };
      layout_ = tbl.at(params.getValueForKey<std::string>("layout"));
    }
    if (params.hasChild("color"))
    {
      color_ = Json::getColorA<float>(params["color"]);
    }
  }

  ~Text() = default;


private:
  void draw(const ci::Rectf& rect, UI::Drawer& drawer) noexcept override
  {
    ci::gl::pushModelMatrix();
    ci::gl::ScopedGlslProg prog(drawer.getFontShader());

    auto& font = drawer.getFont(font_name_);
    auto size  = font.drawSize(text_) * text_size_;

    float x = rect.getX1();
    float y = rect.getY1();
    if (layout_ & LEFT)
    {
    }
    else if (layout_ & CENTER)
    {
      x = (rect.getX1() + rect.getX2() - size.x) * 0.5;
    }
    else if (layout_ & RIGHT)
    {
      x = rect.getX2() - size.x;
    }
    if (layout_ & TOP)
    {
      y = rect.getY2() - size.y;
    }
    else if (layout_ & MIDDLE)
    {
      y = (rect.getY1() + rect.getY2() - size.y) * 0.5;
    }
    else if (layout_ & BOTTOM)
    {
    }

    ci::gl::translate(x, y);
    ci::gl::scale(glm::vec3(text_size_));
    font.draw(text_, glm::vec2(0, 0), color_);
    ci::gl::popModelMatrix();
  }


  void setParam(const std::string& name, const boost::any& value) noexcept override
  {
    // UI::Widget内
    std::map<std::string, std::function<void (const boost::any& v)>> tbl = {
      { "text", [this](const boost::any& v) noexcept
                {
                  text_ = boost::any_cast<const std::string&>(v);
                } },
      { "size", [this](const boost::any& v) noexcept
                 {
                   text_size_ = boost::any_cast<const float>(v);
                 } },
      { "color", [this](const boost::any& v) noexcept
                 {
                   color_ = boost::any_cast<const ci::ColorA&>(v);
                 } }
    };

    tbl.at(name)(value);
  }

};

} }
