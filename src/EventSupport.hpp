#pragma once

//
// Eventの補助関数
//

#include "Event.hpp"
#include "Arguments.hpp"


namespace ngs {

// サウンド再生
void startTimelineSound(Event<Arguments>& event, const ci::JsonTree& params, const std::string& key) noexcept
{
  if (!params.hasChild(key)) return;

  Arguments args {
    { "timeline", params[key] }
  };
  event.signal("SE:timeline", args);
}

}
