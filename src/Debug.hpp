#pragma once

//
// Debug用のコード
//


#if defined (DEBUG)

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
    
    { "h", ci::app::KeyEvent::KEY_h },
    { "i", ci::app::KeyEvent::KEY_i },
    { "j", ci::app::KeyEvent::KEY_j },
    { "k", ci::app::KeyEvent::KEY_k },
    { "l", ci::app::KeyEvent::KEY_l },
    { "m", ci::app::KeyEvent::KEY_m },
    { "n", ci::app::KeyEvent::KEY_n },

    { "o", ci::app::KeyEvent::KEY_o },
    { "p", ci::app::KeyEvent::KEY_p },
    { "q", ci::app::KeyEvent::KEY_q },
    { "r", ci::app::KeyEvent::KEY_r },
    { "s", ci::app::KeyEvent::KEY_s },
    { "t", ci::app::KeyEvent::KEY_t },
    { "u", ci::app::KeyEvent::KEY_u },
    
    { "v", ci::app::KeyEvent::KEY_v },
    { "w", ci::app::KeyEvent::KEY_w },
    { "x", ci::app::KeyEvent::KEY_x },
    { "y", ci::app::KeyEvent::KEY_y },
    { "z", ci::app::KeyEvent::KEY_z },
  };

  for (const auto& p : params)
  {
    const auto& key   = p.getValueAtIndex<std::string>(0);
    const auto& event = p.getValueAtIndex<std::string>(1);

    events.emplace(keys.at(key), event);
  }

  return events;
}

void signalKeyEvent(Event<Arguments>& event, const std::map<int, std::string>& key_events, int key) noexcept
{
  if (!key_events.count(key)) return;

  event.signal(key_events.at(key), Arguments());
}

} }

#endif
