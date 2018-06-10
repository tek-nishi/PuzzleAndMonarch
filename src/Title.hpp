#pragma once

//
// タイトル画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "EventSupport.hpp"
#include "GameCenter.h"


namespace ngs {

class Title
  : public Task
{

public:
  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
        bool first_time, bool saved, bool ranking) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("title.canvas")),
              Params::load(params.getValueForKey<std::string>("title.tweens")))
  {
    {
      auto v = first_time ? "title.se_first"
                          : "title.se";
      startTimelineSound(event, params, v);
    }

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event_.connect("play:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Title:finished", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Game Start!" << std::endl;
                              });
    
    holder_ += event_.connect("credits:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Credits:begin", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Credits." << std::endl;
                              });
    
    holder_ += event_.connect("settings:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Settings:begin", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Settings." << std::endl;
                              });
    
    holder_ += event_.connect("records:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Records:begin", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Records." << std::endl;
                              });
    
    holder_ += event_.connect("ranking:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Ranking:begin", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Records." << std::endl;
                              });

    holder_ += event_.connect("game_center:touch_ended",
                              [this, wipe_delay](const Connection&, const Arguments&)
                              {
                                canvas_.active(false);

                                count_exec_.add(wipe_delay,
                                                [this]()
                                                {
                                                  GameCenter::showBoard([this]()
                                                                        {
                                                                          // 画面の更新を止める
                                                                          event_.signal("App:pending-update", Arguments());
                                                                        },
                                                                        [this]()
                                                                        {
                                                                          // 画面の更新再開
                                                                          event_.signal("App:resume-update", Arguments());
                                                                          canvas_.active(true);
                                                                        });
                                                });
                              });

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "credits");
    setupCommonTweens(event_, holder_, canvas_, "settings");
    setupCommonTweens(event_, holder_, canvas_, "records");
    setupCommonTweens(event_, holder_, canvas_, "ranking");
    setupCommonTweens(event_, holder_, canvas_, "game_center");
    setupCommonTweens(event_, holder_, canvas_, "play");

    if (!saved)
    {
      // Saveデータがない場合関連するボタンを消す
      {
        const auto& widget = canvas_.at("Records");
        widget->enable(false);
      }
    }
    if (!ranking)
    {
      // Rankingに記録がない場合もボタンを消す
      {
        const auto& widget = canvas_.at("Ranking");
        widget->enable(false);
      }
    }

    if (first_time)
    {
      // 起動時は特別
      canvas_.startTween("launch");
    }
    else
    {
      canvas_.startCommonTween("root", "in-from-left");
      canvas_.startTween("start");
    }

    event.signal("Title:begin", Arguments());
  }

  ~Title() = default;
  

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
