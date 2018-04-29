#pragma once

//
// テキスト描画Widget
//

#include "UIWidgetBase.hpp"
#include <glm/gtx/transform.hpp>


namespace ngs { namespace UI {

class Text
  : public WidgetBase
{
  std::string text_;
  // 初期テキスト(動的レイアウトを使わない場合に必要)
  std::string initial_text_;

  std::string font_name_;
  glm::vec2 layout_;
  ci::Color color_ = ci::Color::white();

  // 文字内容で動的にレイアウトを決める
  bool dynamic_layout_;


public:
  Text(const ci::JsonTree& params) noexcept
    : text_(params.getValueForKey<std::string>("text")),
      font_name_(params.getValueForKey<std::string>("font")),
      initial_text_(text_)
  {
    layout_         = Json::getVec(params, "layout", glm::vec2(0.5));
    dynamic_layout_ = Json::getValue(params, "dynamic_layout", true);

    if (params.hasChild("color"))
    {
      color_ = Json::getColor<float>(params["color"]);
    }
  }

  ~Text() = default;


private:
  void draw(const ci::Rectf& rect, UI::Drawer& drawer, float alpha) noexcept override
  {
    auto& font = drawer.getFont(font_name_);
    // rectの高さからサイズを決める
    auto font_scale = rect.getHeight() / font.getSize();
    const auto& t = dynamic_layout_ ? text_
                                    : initial_text_;
    auto size = font.drawSize(t) * font_scale; 
    // レイアウトを計算
    glm::vec2 pos = rect.getUpperLeft() * (glm::vec2(1.0) - layout_) + (rect.getLowerRight() - size) * layout_; 

    ci::gl::ScopedGlslProg prog(drawer.getFontShader());
    ci::gl::ScopedModelMatrix m;
    
    auto mtx = glm::translate(glm::vec3(pos.x, pos.y, 0.0));
    ci::gl::setModelMatrix(mtx);
    ci::gl::scale(glm::vec3(font_scale));
    ci::ColorA color(color_, alpha);
    font.draw(text_, glm::vec2(0), color);
  }


  void setParam(const std::string& name, const boost::any& value) noexcept override
  {
    std::map<std::string, std::function<void (const boost::any& v)>> tbl = {
      { "text", [this](const boost::any& v) noexcept
                {
                  text_ = boost::any_cast<const std::string&>(v);
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
      { "text", [this]() noexcept
                {
                  return &text_;
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
