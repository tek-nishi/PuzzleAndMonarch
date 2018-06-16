#pragma once

//
// UI周りのサポート関数
//

#include "UICanvas.hpp"
#include "CountExec.hpp"


namespace ngs { namespace UI {

void startButtonTween(CountExec& count_exec, UI::Canvas& canvas,
                      double delay, double interval, const std::vector<std::pair<std::string, std::string>>& widgets)
{
  for (const auto& id : widgets)
  {
    // 非表示のWidgetは対象外
    if (!canvas.isEnableWidget(id.first)) continue;

    canvas.enableWidget(id.first, false);
    count_exec.add(delay,
                   [&canvas, id]()
                   {
                     canvas.startCommonTween(id.first, "icon:circle");
                     canvas.startCommonTween(id.second, "icon:text");
                   });
    delay += interval;
  }
}

} }
