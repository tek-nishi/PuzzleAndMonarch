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

  int mode_ = 0;


public:
  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer, params["ui.camera"], Params::load(params.getValueForKey<std::string>("title.canvas")))
  {
    holder_ += event_.connect("play:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(1.0, [this](){ mode_ += 1; });
                                DOUT << "Game Start!" << std::endl;
                              });
  }

  ~Title() = default;
  

  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    switch (mode_)
    {
    case 0:
      {
      }
      break;

    case 1:
      {
        // 終了
        event_.signal("Title:finished", Arguments());
        DOUT << "Title finished." << std::endl;
        return false;
      }
      break;
    }

    return true;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    canvas_.draw();
  }

};

}
