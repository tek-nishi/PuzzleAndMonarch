#pragma once

//
// Settings画面
//

namespace ngs {

class Settings
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool bgm_enable_ = true;
  bool se_enable_  = true;

  bool active_ = true;


public:
  Settings(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer, params["ui.camera"], Params::load(params.getValueForKey<std::string>("settings.canvas")))
  {
    count_exec_.add(1.0,
                    [this]() {
                      const auto& widget = canvas_.at("touch");
                      widget->enable();
                    });

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(1.0, [this]() noexcept
                                                     {
                                                       event_.signal("Settings:Finished", Arguments());
                                                       active_ = false;
                                                     });
                                DOUT << "Back to Title" << std::endl;
                              });

    holder_ += event_.connect("BGM:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                bgm_enable_ = !bgm_enable_;
                                auto text = bgm_enable_ ? u8"" : u8"";
                                const auto& widget = canvas_.at("BGM");
                                widget->setParam("text", std::string(text));
                              });

    holder_ += event_.connect("SE:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                se_enable_ = !se_enable_;
                                auto text = se_enable_ ? u8"" : u8"";
                                const auto& widget = canvas_.at("SE");
                                widget->setParam("text", std::string(text));
                              });
  }

  ~Settings() = default;
  

  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    canvas_.draw();
  }
};

}
