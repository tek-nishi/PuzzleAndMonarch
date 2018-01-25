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

  void setParam(const std::string& name, const boost::any& value) noexcept override
  {
  }


public:
  Brank()  = default;
  ~Brank() = default;

};

} }
