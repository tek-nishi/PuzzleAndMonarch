#pragma once

//
// UIの最上位
//

#include "Params.hpp"
#include "UIWidget.hpp"
#include <map>
#include <string>


namespace ngs { namespace UI {

class Canvas {


public:
  Canvas(const std::string& path, UI::WidgetsFactory& factory) noexcept
    : root_widget_(factory.construct(Params::load(path)))
  {
    // クエリ用に列挙
  }



  void draw(const ci::Rectf& rect, const glm::vec2& scale,
            const ci::gl::GlslProgRef& shader,
            Font& font, const ci::gl::GlslProgRef& font_shader) noexcept
  {
    root_widget_->draw(rect, scale, shader, font, font_shader);
  }




private:




  // メンバ変数を後半で定義する
  WidgetPtr root_widget_;
  std::map<std::string, WidgetPtr> query_widget_;

};

} }

