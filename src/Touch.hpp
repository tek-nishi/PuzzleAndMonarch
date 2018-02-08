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


  Touch(uint32_t id_, bool handled_, const glm::vec2& pos_, const glm::vec2& prev_pos_) noexcept
  : id(id_),
    handled(handled_),
    pos(pos_),
    prev_pos(prev_pos_)
  {}

  ~Touch() = default;
};

}
