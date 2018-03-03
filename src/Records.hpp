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
    UI::Canvas::setWidgetText(canvas_, "record:0",  std::to_string(detail.play_times));
    UI::Canvas::setWidgetText(canvas_, "record:1",  std::to_string(detail.total_panels));
    UI::Canvas::setWidgetText(canvas_, "record:2",  std::to_string(detail.panel_moved_times));
    UI::Canvas::setWidgetText(canvas_, "record:3",  std::to_string(detail.panel_turned_times));
    UI::Canvas::setWidgetText(canvas_, "record:4",  std::to_string(detail.share_times));
    UI::Canvas::setWidgetText(canvas_, "record:5",  std::to_string(detail.startup_times));
    UI::Canvas::setWidgetText(canvas_, "record:6",  std::to_string(detail.abort_times));

    {
      char text[64];
      sprintf(text, "%.0f", detail.average_score);
      UI::Canvas::setWidgetText(canvas_, "record:10", text);
    }
    {
      char text[64];
      sprintf(text, "%.1f", detail.average_put_panels);
      UI::Canvas::setWidgetText(canvas_, "record:11", text);
    }
    {
      char text[64];
      sprintf(text, "%.1f", detail.average_moved_times);
      UI::Canvas::setWidgetText(canvas_, "record:12", text);
    }
    {
      char text[64];
      sprintf(text, "%.1f", detail.average_turn_times);
      UI::Canvas::setWidgetText(canvas_, "record:13", text);
    }
    {
      char text[64];
      sprintf(text, "%.1fs", detail.average_put_time);
      UI::Canvas::setWidgetText(canvas_, "record:14", text);
    }
  }
};

}
