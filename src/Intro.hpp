#pragma once

//
// イントロ画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "EventSupport.hpp"


namespace ngs {

class Intro
  : public Task
{

public:
  Intro(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("intro.canvas")),
              Params::load(params.getValueForKey<std::string>("intro.tweens"))),
      finish_delay_(params.getValueForKey<double>("intro.finish_delay"))
  {
    count_exec_.add(1.0,
                    [this]()
                    {
                      holder_ += event_.connect("single_touch_ended",
                                                [this](const Connection&, const Arguments&)
                                                {
                                                  finishTask();
                                                });
                    });


    event.signal("Intro:begin", Arguments());
    canvas_.startTween("start");
  }

  ~Intro() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    if (tweening_ && !canvas_.hasTween())
    {
      // 演出が完了したらタイトル画面へ
      tweening_ = false;

      count_exec_.add(finish_delay_,
                      [this]()
                      {
                        finishTask();
                      });
    }

    return active_;
  }

  void finishTask()
  {
    if (!active_) return;

    active_ = false;
    event_.signal("Intro:finished", Arguments());
    DOUT << "Intro:finished" << std::endl;
  }




  
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  double finish_delay_;
  bool tweening_ = true;

  bool active_ = true;
};

}
