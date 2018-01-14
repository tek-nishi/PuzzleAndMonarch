#pragma once

//
// Widget描画基本クラス
//

#include "UIDrawer.hpp"


namespace ngs { namespace UI {

struct WidgetBase
{
  virtual ~WidgetBase() {}

  virtual void draw(const ci::Rectf& rect, UI::Drawer& drawer) noexcept = 0;

};

} }
