#pragma once

//
// 結果画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"


namespace ngs {

class Result
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  int mode_ = 0;


public:
  Result(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer, params["ui.camera"], Params::load(params.getValueForKey<std::string>("result.canvas")))
  {
  }

  ~Result() = default;


  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return true;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    canvas_.draw();
  }
};

}
