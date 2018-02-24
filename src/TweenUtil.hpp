#pragma once

//
// Tween補助
//

#include <boost/any.hpp>
#include "Event.hpp"
#include "Arguments.hpp"
#include "ConnectionHolder.hpp"
#include "UICanvas.hpp"


namespace ngs
{

void setupCommonTweens(Event<Arguments>& event, ConnectionHolder& holder, UI::Canvas& canvas,
                       const std::string& event_name) noexcept
{
  static const char* identifiers[] = {
    ":touch_began",
    ":moved_out",
    ":moved_in",
    ":touch_ended",
  };
  static const char* tween_name[] = {
    "touch-in",
    "moved-out",
    "moved-in",
    "ended-in",
  };

  for (int i = 0; i < 4; ++i)
  {
    const auto& id = identifiers[i];
    const auto& tn = tween_name[i];

    holder += event.connect(event_name + id,
                            [&canvas, &tn](const Connection&, const Arguments& arg) noexcept
                            {
                              if (!arg.count("widget")) return;

                              const auto& widget = boost::any_cast<const std::string&>(arg.at("widget"));
                              canvas.startCommonTween(widget, tn);
                            });
  }
}

}
