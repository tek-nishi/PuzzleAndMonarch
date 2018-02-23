#pragma once

//
// UI::WidgetsをJSONから生成
//

#include "UIWidget.hpp"
#include "UIBrank.hpp"
#include "UIText.hpp"
#include "UIRoundRect.hpp"
#include "UICircle.hpp"
#include "UIRect.hpp"


namespace ngs { namespace UI {

class WidgetsFactory
  : private boost::noncopyable
{
  // JSONから生成(階層構造も含む)
  WidgetPtr create(const ci::JsonTree& params) const noexcept
  {
    auto widget = UI::Widget::createFromParams(params);

    // タイプ別
    if (params.hasChild("text"))
    {
      auto base = std::make_unique<UI::Text>(params);
      widget->setWidgetBase(std::move(base));
    }
    else if (params.hasChild("corner_radius"))
    {
      auto base = std::make_unique<UI::RoundRect>(params);
      widget->setWidgetBase(std::move(base));
    }
    else if (params.hasChild("radius"))
    {
      auto base = std::make_unique<UI::Circle>(params);
      widget->setWidgetBase(std::move(base));
    }
    else if (params.hasChild("color"))
    {
      auto base = std::make_unique<UI::Rect>(params);
      widget->setWidgetBase(std::move(base));
    }
    else
    {
      auto base = std::make_unique<UI::Brank>();
      widget->setWidgetBase(std::move(base));
    }

    // 子供を追加
    // TIPS:再帰で実装
    if (params.hasChild("childlen"))
    {
      for (const auto& child : params["childlen"])
      {
        widget->addChildren(create(child));
      }
    }
    
    return widget;
  }


  
public:
  WidgetsFactory()  = default; 
  ~WidgetsFactory() = default;


  // JSONからWidgetを生成する
  WidgetPtr construct(const ci::JsonTree& params) noexcept
  {
    auto widget = create(params);
    Widget::checkInactiveWidget(widget);

    return widget;
  }
  
};

} }
