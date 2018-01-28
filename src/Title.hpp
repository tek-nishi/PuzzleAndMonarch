#pragma once

//
// タイトル画面
//

#include "Task.hpp"
#include "UICanvas.hpp"


namespace ngs {

class Title
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;
  
  UI::Canvas canvas_;

  int mode_ = 0;


public:
  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("title.canvas")))
  {
    holder_ += event_.connect("start:touch_ended",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                canvas_.active(false);
                                mode_ += 1;
                                DOUT << "Game Start!" << std::endl;
                              });
  }

  ~Title() = default;
  

  bool update(const double current_time, const double delta_time) noexcept override
  {
    const auto& widget = canvas_.at("5");
    switch (mode_)
    {
    case 0:
      {
        float alpha = std::abs(std::sin(M_PI * current_time));
        ci::ColorA color(1, 1, 1, alpha);
        widget->setParam("color", color);
      }
      break;

    case 1:
      {
        float alpha = std::abs(std::sin(M_PI * 4.0 * current_time));
        ci::ColorA color(1, 1, 1, alpha);
        widget->setParam("color", color);
      }
      break;
    }

    return true;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    ci::gl::enableDepth(false);
    ci::gl::disable(GL_CULL_FACE);
    ci::gl::enableAlphaBlending();

    canvas_.draw();
  }

};

}
