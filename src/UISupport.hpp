#pragma once

//
// UI周りのサポート関数
//

#include "UICanvas.hpp"
#include "CountExec.hpp"


namespace ngs { namespace UI {

void startButtonTween(CountExec& count_exec, UI::Canvas& canvas,
                      double delay, double interval, const std::vector<std::string>& widgets)
{
  for (const auto& name : widgets)
  {
    // 非表示のWidgetは対象外
    if (!canvas.isEnableWidget(name)) continue;

    canvas.enableWidget(name, false);
    count_exec.add(delay,
                   [&canvas, name]()
                   {
                     canvas.startCommonTween(name, "icon:circle");
                     canvas.startCommonTween(name + ":icon", "icon:text");
                   });
    delay += interval;
  }
}

} }
