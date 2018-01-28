#pragma once

//
// ゲーム本編UI
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"


namespace ngs {

class GameMain
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  int mode_ = 0;


public:
  GameMain(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer, params["ui.camera"], Params::load(params.getValueForKey<std::string>("gamemain.canvas")))
  {
    count_exec_.add(2.0, [this]() {
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
                              [this](const Connection&, const Arguments& arg) noexcept
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
                              [this](const Connection&, const Arguments& arg) noexcept
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
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                // ゲーム終了
                                event_.signal("GameMain:aborted", Arguments());
                                DOUT << "GameMain finished." << std::endl;
                              });
  }

  ~GameMain() = default;


  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return true;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    canvas_.draw();
  }

};

}
