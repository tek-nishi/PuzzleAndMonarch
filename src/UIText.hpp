#pragma once

//
// テキスト描画Widget
//

#include "UIWidgetBase.hpp"


namespace ngs { namespace UI {

class Text
  : public WidgetBase
{
  std::string text_;

  std::string font_name_;
  float text_size_ = 1.0f;
  glm::vec2 layout_ = { 0.5, 0.5 };
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
      layout_ = Json::getVec<glm::vec2>(params["layout"]);
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
    // FontのサイズとWidgetのサイズからスケーリングを計算
    auto font_scale = text_size_ / font.getSize();
    auto size  = font.drawSize(text_) * font_scale; 

    glm::vec2 pos = rect.getUpperLeft() * (glm::vec2(1.0) - layout_) + (rect.getLowerRight() - size) * layout_; 
    ci::gl::translate(pos);
    ci::gl::scale(glm::vec3(font_scale));
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
