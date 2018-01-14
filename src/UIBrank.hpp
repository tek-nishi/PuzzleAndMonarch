#pragma once

//
// 何もしないUI
//

#include "UIWidgetBase.hpp"


namespace ngs { namespace UI {

class Brank
  : public WidgetBase
{

  void draw(const ci::Rectf& rect, UI::Drawer& drawer) noexcept override
  {
  }


public:
  Brank()  = default;
  ~Brank() = default;

};

} }
