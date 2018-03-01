#pragma once

//
// タイトル画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"


namespace ngs {

class Title
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;



public:
  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
        bool first_time, bool saved) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("title.canvas")),
              Params::load(params.getValueForKey<std::string>("title.tweens")))
  {
    {
      auto v = first_time ? "title.se_first"
                          : "title.se";

      Arguments args {
        { "timeline", params[v] }
      };
      event_.signal("SE:timeline", args);
    }

    holder_ += event_.connect("play:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("begin_game");
                                count_exec_.add(0.5,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Title:finished", Arguments());
                                                });
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Game Start!" << std::endl;
                              });
    
    holder_ += event_.connect("credits:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Credits:begin", Arguments());
                                                  active_ = false;
                                                });
                                DOUT << "Credits." << std::endl;
                              });
    
    holder_ += event_.connect("settings:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(0.6,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Settings:begin", Arguments());
                                                });
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Settings." << std::endl;
                              });
    
    holder_ += event_.connect("records:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Records:begin", Arguments());
                                                  active_ = false;
                                                });
                                DOUT << "Records." << std::endl;
                              });
    
    holder_ += event_.connect("ranking:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Ranking:begin", Arguments());
                                                  active_ = false;
                                                });
                                DOUT << "Records." << std::endl;
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
        const auto& widget = canvas_.at("Ranking");
        widget->enable(false);
      }
      {
        const auto& widget = canvas_.at("Records");
        widget->enable(false);
      }
    }

    canvas_.startTween(first_time ? "launch" : "start");
  }

  ~Title() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }

};

}
