#pragma once

//
// タイトル画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "EventSupport.hpp"
#include "UISupport.hpp"
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
      canvas_.enableWidget("Records", false);
    }
    if (!ranking)
    {
      // Rankingに記録がない場合もボタンを消す
      canvas_.enableWidget("Ranking", false);
    }
    if (GameCenter::isAuthenticated())
    {
      // GameCenter
      game_center_ = true;
      canvas_.enableWidget("GameCenter");
    }

    layoutIcons(params);

    if (first_time)
    {
      // 起動時は特別
      canvas_.startTween("launch");
      // startIconTweens(params);
      startMainTween(params, 2.5);
    }
    else
    {
      canvas_.startCommonTween("root", "in-from-left");
      canvas_.startTween("start");
      startMainTween(params, 0.5);
    }

    event.signal("Title:begin", Arguments());
  }

  ~Title() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    // GameCenterへのログイン状態が変化した
    if (GameCenter::isAuthenticated() != game_center_)
    {
      game_center_ = GameCenter::isAuthenticated();
      if (game_center_)
      {
        canvas_.startTween("GameCenterOn");
        DOUT << "GameCenter On" << std::endl;
      }
      else
      {
        canvas_.startTween("GameCenterOff");
        DOUT << "GameCenter Off" << std::endl;
      }
    }

    return active_;
  }

  // アイコンを再レイアウト
  void layoutIcons(const ci::JsonTree& params)
  {
    const auto& menu = params["title.menu"];

    // 有効なアイコンを数える
    int num = 0;
    for (const auto& w : menu)
    {
      const auto& name = w.getValue<std::string>();
      if (canvas_.isEnableWidget(name)) num += 1;
    }

    // FIXME マジックナンバー
    float x = -(20 * num + 15 * (num - 1)) * 0.5f;
    for (const auto& w : menu)
    {
      const auto& name = w.getValue<std::string>();
      if (!canvas_.isEnableWidget(name)) continue;

      // NOTICE Rectを修正しているのでやや煩雑
      auto p = canvas_.getWidgetParam(name, "rect");
      auto* rect = boost::any_cast<ci::Rectf*>(p);
      rect->x1 = x;
      rect->x2 = x + 20;

      x += 20 + 15;
    }
  }

  // アイコンの演出開始
  void startIconTweens(const ci::JsonTree& params)
  {
    const auto& menu = params["title.menu"];
    double delay = 2.5;
    for (const auto& w : menu)
    {
      const auto& name = w.getValue<std::string>();
      if (!canvas_.isEnableWidget(name)) continue;

      canvas_.enableWidget(name, false);
      count_exec_.add(delay,
                      [this, name]()
                      {
                        canvas_.startCommonTween(name, "icon:circle");
                        canvas_.startCommonTween(name + "Icon", "icon:text");
                      });
      delay += 0.15;
    }
  }

  // メイン画面のボタン演出
  void startMainTween(const ci::JsonTree& params, double delay)
  {
    std::vector<std::pair<std::string, std::string>> widgets;
    const auto& menu = params["title.menu"];
    for (const auto m : menu)
    {
      const auto& id = m.getValue<std::string>();
      widgets.push_back({ id, id + ":icon" });
    }

    UI::startButtonTween(count_exec_, canvas_, delay, 0.15, widgets);
  }


  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;

  bool game_center_ = false;

};

}
