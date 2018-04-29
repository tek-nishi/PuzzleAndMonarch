#pragma once

//
// チュートリアル
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "EventSupport.hpp"


namespace ngs {

class Tutorial 
  : public Task
{

public:
  Tutorial(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("tutorial.canvas")),
              Params::load(params.getValueForKey<std::string>("tutorial.tweens")))
  {
    canvas_.startTween("start");
  }

  ~Tutorial() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }


  
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;
};

}
