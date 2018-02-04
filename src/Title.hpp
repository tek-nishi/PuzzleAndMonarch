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

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "credits");
    setupCommonTweens(event_, holder_, canvas_, "settings");
    setupCommonTweens(event_, holder_, canvas_, "game_center");
    setupCommonTweens(event_, holder_, canvas_, "play");

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


  static void setupCommonTweens(Event<Arguments>& event, ConnectionHolder& holder, UI::Canvas& canvas,
                                const std::string& event_name) noexcept
  {
    static const char* identifiers[] = {
      ":touch_began",
      ":moved_out",
      ":moved_in",
      ":touch_ended",
    };
    static const char* tween_name[] = {
      "touch-in",
      "moved-out",
      "moved-in",
      "ended-in",
    };

    for (int i = 0; i < 4; ++i)
    {
      const auto& id = identifiers[i];
      const auto& tn = tween_name[i];

      holder += event.connect(event_name + id,
                              [&canvas, &tn](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& widget = boost::any_cast<const std::string&>(arg.at("widget"));
                                canvas.startCommonTween(widget, tn);
                              });
    }
  }

};

}
