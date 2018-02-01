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
