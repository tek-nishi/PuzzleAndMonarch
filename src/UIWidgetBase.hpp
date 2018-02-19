#pragma once

//
// Widget描画基本クラス
//

#include <boost/any.hpp>
#include "UIDrawer.hpp"


namespace ngs { namespace UI {

struct WidgetBase
  : private boost::noncopyable
{
  virtual ~WidgetBase() = default;

  virtual void draw(const ci::Rectf& rect, UI::Drawer& drawer, float alpha) noexcept = 0;

  virtual void setParam(const std::string& name, const boost::any& values) noexcept = 0; 
  virtual boost::any getParam(const std::string& name) noexcept = 0; 

};

} }
