#pragma once

//
// プレイ記録
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"


namespace ngs {

class Records
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;


public:
  // FIXME 受け渡しが冗長なのを解決したい
  struct Detail {
    u_int play_times;
    u_int high_score;
    u_int total_panels;
  };


  Records(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
          const Detail& detail) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("records.canvas")),
              Params::load(params.getValueForKey<std::string>("records.tweens")))
  {
    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Records:Finished", Arguments());
                                                  active_ = false;
                                                });
                                DOUT << "Back to Title" << std::endl;
                              });

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "agree");

    applyDetail(detail);
    canvas_.startTween("start");
  }

  ~Records() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }


  void applyDetail(const Detail& detail) noexcept
  {
    {
      const auto& widget = canvas_.at("record:0");
      widget->setParam("text", std::to_string(detail.play_times));
    }
    {
      const auto& widget = canvas_.at("record:1");
      widget->setParam("text", std::to_string(detail.high_score));
    }
    {
      const auto& widget = canvas_.at("record:2");
      widget->setParam("text", std::to_string(detail.total_panels));
    }
  }
};

}
