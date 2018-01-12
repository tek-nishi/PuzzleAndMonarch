#pragma once

//
// UI::WidgetsをJSONから生成
//

#include "UIWidget.hpp"


namespace ngs { namespace UI {

class WidgetsFactory
  : private boost::noncopyable
{
  // JSONから生成(階層構造も含む)
  WidgetPtr create(const ci::JsonTree& params) noexcept
  {
    auto identifier = params.getValueForKey<std::string>("identifier");
    auto rect = Json::getRect(params["rect"]);
    auto widget = std::make_shared<UI::Widget>(identifier, rect);

    // アンカー
    if (params.hasChild("anchor"))
    {
      widget->setAnchor(Json::getVec<glm::vec2>(params["anchor"][0]),
                        Json::getVec<glm::vec2>(params["anchor"][1]));
    }
    
    // スケーリング
    if (params.hasChild("scale"))
    {
      widget->setScale(Json::getVec<glm::vec2>(params["scale"]));
    }

    if (params.hasChild("pivot"))
    {
      widget->setPivot(Json::getVec<glm::vec2>(params["pivot"]));
    }

    if (params.hasChild("color"))
    {
      widget->setColor(Json::getColorA<float>(params["color"]));
    }
    
    if (params.hasChild("text"))
    {
      widget->setText(params.getValueForKey<std::string>("text"));
    }
    if (params.hasChild("text_size"))
    {
      widget->setTextSize(params.getValueForKey<float>("text_size"));
    }
    if (params.hasChild("alignment"))
    {
      widget->setAlignment(Json::getVec<glm::vec2>(params["alignment"]));
    }


    // 子供を追加
    // TIPS:再帰で実装
    if (params.hasChild("childlen"))
    {
      for (const auto& child : params["childlen"])
      {
        widget->addChild(create(child));
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
    return create(params);
  }
  
};

} }
