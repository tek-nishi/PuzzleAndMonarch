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
#include "PurchaseDelegate.h"


namespace ngs {

class Title
  : public Task
{

public:
  // 生成時の諸々の条件
  struct Condition
  {
    // Introから遷移
    bool first_time;
    // １回以上プレイした
    bool saved;
    // ランキングに記録がある
    bool ranking;
    // 課金した
    bool purchased;

    // チュートリアルレベル
    int tutorial_level;
  };


  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
        const Condition& condition) noexcept
    : event_(event),
      effect_speed_(params.getValueForKey<double>("title.effect_speed")),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("title.canvas")),
              Params::load(params.getValueForKey<std::string>("title.tweens")))
  {
    {
      auto v = condition.first_time ? "title.se_first"
                                    : "title.se";
      startTimelineSound(event, params, v);
    }

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    // FIXME 同じ処理が並んでいる
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

    holder_ += event_.connect("purchase:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Purchase:begin", Arguments());
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

#if defined (DEBUG)
    holder_ += event_.connect("debug-gamecenter",
                              [this](const Connection&, const Arguments&)
                              {
                                force_game_center_ = !force_game_center_;
                              });
#endif

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "credits");
    setupCommonTweens(event_, holder_, canvas_, "settings");
    setupCommonTweens(event_, holder_, canvas_, "records");
    setupCommonTweens(event_, holder_, canvas_, "ranking");
    setupCommonTweens(event_, holder_, canvas_, "game_center");
    setupCommonTweens(event_, holder_, canvas_, "purchase");
    setupCommonTweens(event_, holder_, canvas_, "play");

    auto is_tutorial = condition.tutorial_level >= 0;

    if (!condition.saved || is_tutorial) 
    {
      // Saveデータがない場合関連するボタンを消す
      canvas_.enableWidget("Records", false);
    }
    if (!condition.ranking || is_tutorial)
    {
      // Rankingに記録がない場合もボタンを消す
      canvas_.enableWidget("Ranking", false);
    }
    if (condition.purchased)
    {
      purchased_ = true;
      canvas_.enableWidget("purchased");
    }
    if (is_tutorial)
    {
      canvas_.enableWidget("Credits", false);
    }

#if defined (CINDER_COCOA_TOUCH)
    if (!is_tutorial)
    {
      // GameCenter
      canvas_.enableWidget("GameCenter");
    }
#endif

    changePlayIcon(condition.tutorial_level, params);
    layoutIcons(params);

    if (condition.first_time)
    {
      // 起動時は特別
      canvas_.startTween("launch");
      startMainTween(params, 2.5);
    }
    else
    {
      canvas_.startCommonTween("root", "in-from-left");
      canvas_.startTween("start");
      startMainTween(params, 0.6);
    }

    if (condition.tutorial_level >= 0)
    {
      const auto* id = condition.first_time ? "tutorial-first"
                                            : "tutorial";
      canvas_.startTween(id);
    }

    event.signal("Title:begin", Arguments());
  }

  ~Title() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    if (purchased_)
    {
      // 課金時の演出
      auto color = ci::hsvToRgb({ std::fmod(current_time * effect_speed_, 1.0), 0.6f, 1 });
      canvas_.setWidgetParam("logo-icon", "color", color);
    }

    // GameCenterへのログイン状態が変化した
    bool gamecenter = GameCenter::isAuthenticated();
#if defined (DEBUG)
    gamecenter |= force_game_center_;
#endif
    if (gamecenter != game_center_)
    {
      game_center_ = gamecenter;
      canvas_.activeWidget("GameCenter", gamecenter);
      if (gamecenter)
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

    // 課金の値段情報ゲット!!
    bool purchase = PurchaseDelegate::hasPrice();
    if (purchase != has_purchase_)
    {
      has_purchase_ = purchase;
      canvas_.activeWidget("Purchase", purchase);
      if (purchase)
      {
        canvas_.startTween("PurchaseOn");
        DOUT << "Purchase On" << std::endl;
      }
      else
      {
        canvas_.startTween("PurchaseOff");
        DOUT << "Purchase Off" << std::endl;
      }
    }

    return active_;
  }

  // Playアイコンの変更
  void changePlayIcon(int tutorial_level, const ci::JsonTree& params)
  {
    auto& p = (tutorial_level >= 0) ? params["title.tutorial-icon"]
                                    : params["title.play-icon"];
    canvas_.setWidgetText("play:icon", p.getValueForKey<std::string>("text"));

    auto anchor_min = Json::getVec<glm::vec2>(p["anchor"][0]);
    auto anchor_max = Json::getVec<glm::vec2>(p["anchor"][1]);
    canvas_.setWidgetParam("play:icon", "anchor_min", anchor_min);
    canvas_.setWidgetParam("play:icon", "anchor_max", anchor_max);

    canvas_.setWidgetText("tutorial-comp", std::to_string(tutorial_level));
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

    UI::startButtonTween(count_exec_, canvas_, delay, 0.18, widgets);
  }


  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;

  double effect_speed_;

  bool game_center_ = false;
  bool has_purchase_ = false;
  bool purchased_ = false;

#if defined (DEBUG)
  // GameCenter強制ON機能
  bool force_game_center_ = false;
#endif

};

}
