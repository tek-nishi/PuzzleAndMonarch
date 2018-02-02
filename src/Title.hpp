#pragma once

//
// タイトル画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"


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
  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("title.canvas")),
              Params::load(params.getValueForKey<std::string>("title.tweens")))
  {
    holder_ += event_.connect("play:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(1.0, [this]() noexcept
                                                     {
                                                       event_.signal("Title:finished", Arguments());
                                                       active_ = false;
                                                     });
                                DOUT << "Game Start!" << std::endl;
                              });
    
    holder_ += event_.connect("credits:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(1.0, [this]() noexcept
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
                                count_exec_.add(1.0, [this]() noexcept
                                                     {
                                                       event_.signal("Settings:begin", Arguments());
                                                       active_ = false;
                                                     });
                                DOUT << "Settings." << std::endl;
                              });

    holder_ += event_.connect("credits:touch_began",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& widget = boost::any_cast<const std::string&>(arg.at("widget"));
                                canvas_.startCommonTween(widget, "touch-in");
                              });
    holder_ += event_.connect("credits:moved_out",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& widget = boost::any_cast<const std::string&>(arg.at("widget"));
                                canvas_.startCommonTween(widget, "moved-out");
                              });
    holder_ += event_.connect("credits:moved_in",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& widget = boost::any_cast<const std::string&>(arg.at("widget"));
                                canvas_.startCommonTween(widget, "moved-in");
                              });
    holder_ += event_.connect("credits:touch_ended",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& widget = boost::any_cast<const std::string&>(arg.at("widget"));
                                canvas_.startCommonTween(widget, "ended-in");
                              });

    canvas_.startTween("start");
  }

  ~Title() = default;
  

  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    canvas_.update(delta_time);

    return active_;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    canvas_.draw();
  }

};

}
