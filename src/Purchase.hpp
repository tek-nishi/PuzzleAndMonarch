#pragma once

//
// 課金画面
//

namespace ngs {

class Purchase
  : public Task
{

public:
  Purchase(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common)
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("purchase.canvas")),
              Params::load(params.getValueForKey<std::string>("purchase.tweens")))
  {
    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event.connect("agree:touch_ended",
                             [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                             {
                               canvas_.active(false);
                               canvas_.startCommonTween("root", "out-to-right");
                               count_exec_.add(wipe_delay,
                                               [this]() noexcept
                                               {
                                                 event_.signal("Purchase:Finished", Arguments());
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 active_ = false;
                                               });
                               DOUT << "Back to Title" << std::endl;
                             });

    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "Purchase");
    setupCommonTweens(event_, holder_, canvas_, "Restore");

    canvas_.startCommonTween("root", "in-from-right");
  }

  ~Purchase() = default;




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
