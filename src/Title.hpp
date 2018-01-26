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
  
  float frame_count = 0;


public:
  Title(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("ui_test.canvas.widgets")))
  {
  }

  ~Title() = default;
  

  bool update() noexcept override
  {
    frame_count += 1.0f;

    const auto& widget = canvas_.at("5");

    float alpha = std::abs(std::sin(frame_count * 0.04));
    ci::ColorA color(1, 1, 1, alpha);

    widget->setParam("color", color);

    if (frame_count > 120.0f) return false;

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
