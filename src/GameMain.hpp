#pragma once

//
// ゲーム本編UI
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"


namespace ngs {

class GameMain
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool active_ = true;


public:
  GameMain(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("gamemain.canvas")),
              Params::load(params.getValueForKey<std::string>("gamemain.tweens")))
  {
    // ゲーム開始
    count_exec_.add(2.0, [this]() noexcept
                         {
                           event_.signal("Game:Start", Arguments());
                           {
                             const auto& widget = canvas_.at("begin");
                             widget->enable(false);
                           }
                           {
                             const auto& widget = canvas_.at("main");
                             widget->enable();
                           }
                         });

    holder_ += event_.connect("pause:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                {
                                  const auto& widget = canvas_.at("main");
                                  widget->enable(false);
                                }
                                {
                                  const auto& widget = canvas_.at("pause_menu");
                                  widget->enable();
                                }
                                event_.signal("GameMain:pause", Arguments());
                              });
    
    holder_ += event_.connect("resume:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                {
                                  const auto& widget = canvas_.at("pause_menu");
                                  widget->enable(false);
                                }
                                {
                                  const auto& widget = canvas_.at("main");
                                  widget->enable();
                                }
                                event_.signal("GameMain:resume", Arguments());
                              });
    
    holder_ += event_.connect("abort:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // ゲーム終了
                                event_.signal("Game:Aborted", Arguments());
                                DOUT << "GameMain finished." << std::endl;
                              });

    // UI更新
    holder_ += event_.connect("Game:UI",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                char text[100];
                                u_int remainig_time = std::ceil(boost::any_cast<double>(arg.at("remaining_time")));
                                u_int minutes = remainig_time / 60;
                                u_int seconds = remainig_time % 60;
                                sprintf(text, "%d'%02d", minutes, seconds);

                                const auto& widget = canvas_.at("time_remain");
                                widget->setParam("text", std::string(text));

                                // 時間が10秒切ったら色を変える
                                ci::ColorA color(1, 1, 1, 1);
                                if (remainig_time <= 10) {
                                  color = ci::ColorA(1, 0, 0, 1);
                                }
                                widget->setParam("color", color);
                              });

    holder_ += event_.connect("Game:UpdateScores",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                const auto& scores = boost::any_cast<std::vector<u_int>>(args.at("scores"));
                                updateScores(scores);
                              });


    // ゲーム完了
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                {
                                  const auto& widget = canvas_.at("main");
                                  widget->enable(false);
                                }
                                {
                                  const auto& widget = canvas_.at("end");
                                  widget->enable();
                                }
                                count_exec_.add(1.5,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                              });

    setupCommonTweens(event_, holder_, canvas_, "pause");
    setupCommonTweens(event_, holder_, canvas_, "resume");
    setupCommonTweens(event_, holder_, canvas_, "abort");
  }

  ~GameMain() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }

  void updateScores(const std::vector<u_int>& scores) noexcept
  {
    int i = 1;
    for (auto score : scores)
    {
      char id[16];
      std::sprintf(id, "score:%d", i);

      const auto& widget = canvas_.at(id);
      widget->setParam("text", std::to_string(score));

      i += 1;
    }
  }

};

}
