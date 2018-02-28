#pragma once

//
// 何もしないUI
//

#include "UIWidgetBase.hpp"


namespace ngs { namespace UI {

class Brank
  : public WidgetBase
{

  ci::Rectf draw(const ci::Rectf& rect, UI::Drawer& drawer, float alpha) noexcept override
  {
    return rect;
  }

  void setParam(const std::string& name, const boost::any& value) noexcept override
  {
  }

  boost::any getParam(const std::string& name) noexcept override
  {
    assert(0);
    return nullptr;
  }


public:
  Brank()  = default;
  ~Brank() = default;

};

} }
