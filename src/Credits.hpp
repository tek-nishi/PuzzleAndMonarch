#pragma once

//
// Credits画面
//

#include "TweenUtil.hpp"


namespace ngs {

class Credits
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;


public:
  Credits(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("credits.canvas")),
              Params::load(params.getValueForKey<std::string>("credits.tweens")))
  {
    // count_exec_.add(1.0,
    //                 [this]() {
    //                   const auto& widget = canvas_.at("touch");
    //                   widget->enable();
    //                 });

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(1.0,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Credits:Finished", Arguments());
                                                  active_ = false;
                                                });
                                DOUT << "Back to Title" << std::endl;
                              });

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "agree");

    canvas_.startTween("start");
  }

  ~Credits() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }

};

}
