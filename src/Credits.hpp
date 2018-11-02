#pragma once

//
// Credits画面
//

#include "TweenUtil.hpp"
#include "UISupport.hpp"


namespace ngs {

class Credits
  : public Task
{
public:
  Credits(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("credits.canvas")),
              Params::load(params.getValueForKey<std::string>("credits.tweens")))
  {
    startTimelineSound(event_, params, "credits.se");

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event_.connect("agree:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Credits:Finished", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Back to Title" << std::endl;
                              });

    auto url = params.getValueForKey<std::string>("credits.privacy");
    holder_ += event_.connect("privacy:touch_ended",
                              [this, url, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(wipe_delay,
                                                [this, url]() noexcept
                                                {
                                                  Os::openURL(url);
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active(true);
                                                });
                              });

#if defined (CINDER_MSW)
    {
      canvas_.enableWidget("privacy", false);
      // ボタンのレイアウト変更
      glm::vec2 ofs;
      canvas_.setWidgetParam("touch", "offset", ofs);
    }
#endif

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "privacy");
    canvas_.startCommonTween("root", "in-from-right");

    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "touch",   "touch:icon" },
      { "privacy", "privacy:icon" }
    };
    UI::startButtonTween(count_exec_, canvas_, 0.55, 0.2, widgets);
  }

  ~Credits() = default;
  

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
