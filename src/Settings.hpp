#pragma once

//
// Settings画面
//

#include "TweenUtil.hpp"


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
  // FIXME 受け渡しが冗長なのを解決したい
  struct Detail {
    bool bgm_enable;
    bool se_enable;
  };


  Settings(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
           const Detail& detail) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("settings.canvas")),
              Params::load(params.getValueForKey<std::string>("settings.tweens")))
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
                                                       Arguments args = {
                                                         { "bgm-enable", bgm_enable_ },
                                                         { "se-enable",  se_enable_ }
                                                       };
                                                       event_.signal("Settings:Finished", args);
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

                                Arguments args = {
                                  { "bgm-enable", bgm_enable_ },
                                  { "se-enable",  se_enable_ }
                                };
                                event_.signal("Settings:Changed", args);
                              });

    holder_ += event_.connect("SE:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                se_enable_ = !se_enable_;
                                auto text = se_enable_ ? u8"" : u8"";
                                const auto& widget = canvas_.at("SE");
                                widget->setParam("text", std::string(text));

                                Arguments args = {
                                  { "bgm-enable", bgm_enable_ },
                                  { "se-enable",  se_enable_ }
                                };
                                event_.signal("Settings:Changed", args);
                              });

    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "BGM");
    setupCommonTweens(event_, holder_, canvas_, "SE");

    applyDetail(detail);
  }

  ~Settings() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }


  void applyDetail(const Detail& detail) noexcept
  {
    bgm_enable_ = detail.bgm_enable;
    se_enable_  = detail.se_enable;

    {
      auto text = bgm_enable_ ? u8"" : u8"";
      const auto& widget = canvas_.at("BGM");
      widget->setParam("text", std::string(text));
    }
    {
      auto text = se_enable_ ? u8"" : u8"";
      const auto& widget = canvas_.at("SE");
      widget->setParam("text", std::string(text));
    }
  }
};

}
