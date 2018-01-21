#pragma once

//
// タッチ
//


namespace ngs {

struct Touch
{
  uint32_t id;
  bool handled;

  glm::vec2 pos;
  glm::vec2 prev_pos;
};

}
