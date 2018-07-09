#pragma once

//
// プレイ記録
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "UISupport.hpp"


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
    u_int max_panels;
    u_int total_panels;
    u_int panel_turned_times;
    u_int panel_moved_times;
    u_int share_times;
    u_int startup_times;
    u_int abort_times;

    double average_score;
    double average_put_panels;
    double average_moved_times;
    double average_turn_times;
    double average_put_time;
  };


  Records(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
          const Detail& detail) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("records.canvas")),
              Params::load(params.getValueForKey<std::string>("records.tweens")))
  {
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
                                                  event_.signal("Records:Finished", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Back to Title" << std::endl;
                              });

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "agree");

    applyDetail(detail);
    canvas_.startCommonTween("root", "in-from-right");

    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "touch", "touch:icon" },
    };
    UI::startButtonTween(count_exec_, canvas_, 0.55, 0.0, widgets);
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
    canvas_.setWidgetText("record:0",  std::to_string(detail.play_times));
    canvas_.setWidgetText("record:1",  std::to_string(detail.max_panels));
    canvas_.setWidgetText("record:2",  std::to_string(detail.total_panels));
    canvas_.setWidgetText("record:3",  std::to_string(detail.panel_moved_times));
    canvas_.setWidgetText("record:4",  std::to_string(detail.panel_turned_times));
    canvas_.setWidgetText("record:5",  std::to_string(detail.share_times));
    canvas_.setWidgetText("record:6",  std::to_string(detail.startup_times));
    canvas_.setWidgetText("record:7",  std::to_string(detail.abort_times));

    {
      char text[64];
      sprintf(text, "%.0f", detail.average_score);
      canvas_.setWidgetText("record:10", text);
    }
    {
      char text[64];
      sprintf(text, "%.1f", detail.average_put_panels);
      canvas_.setWidgetText("record:11", text);
    }
    {
      char text[64];
      sprintf(text, "%.1f", detail.average_moved_times);
      canvas_.setWidgetText("record:12", text);
    }
    {
      char text[64];
      sprintf(text, "%.1f", detail.average_turn_times);
      canvas_.setWidgetText("record:13", text);
    }
    {
      char text[64];
      sprintf(text, "%.1fs", detail.average_put_time);
      canvas_.setWidgetText("record:14", text);
    }
  }
};

}
