#pragma once

//
// Debug用のコード
//


namespace ngs { namespace Debug {

std::map<int, std::string> keyEvent(const ci::JsonTree& params) noexcept
{
  std::map<int, std::string> events;

  static const std::map<std::string, int> keys = {
    { "a", ci::app::KeyEvent::KEY_a },
    { "b", ci::app::KeyEvent::KEY_b },
    { "c", ci::app::KeyEvent::KEY_c },
    { "d", ci::app::KeyEvent::KEY_d },
    { "e", ci::app::KeyEvent::KEY_e },
    { "f", ci::app::KeyEvent::KEY_f },
    { "g", ci::app::KeyEvent::KEY_g },
  };

  for (const auto& p : params)
  {
    const auto& key   = p.getValueAtIndex<std::string>(0);
    const auto& event = p.getValueAtIndex<std::string>(1);

    events.emplace(keys.at(key), event);
  }

  return events;
}

void signalKeyEvent(Event<Arguments>& event, const std::map<int, std::string>& key_events, const int key) noexcept
{
  if (!key_events.count(key)) return;

  event.signal(key_events.at(key), Arguments());
}

} }
