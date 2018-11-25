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
  if (!params.hasChild(key))
  {
    DOUT << "No timeline sound: " << key << std::endl;
    return;
  }

  using namespace std::literals;

  Arguments args {
    { "timeline"s, params[key] }
  };
  event.signal("SE:timeline"s, args);
}

}
