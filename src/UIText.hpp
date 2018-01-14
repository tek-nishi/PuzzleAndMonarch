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
  float text_size_ = 1.0f;
  glm::vec2 alignment_ = { 0.5, 0.5 };
  ci::ColorA color_ = { 1.0f, 1.0f, 1.0f, 1.0f };


  void draw(const glm::vec2& rect, UI::Drawer& drawer) noexcept override
  {

  }

};

} }
