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
    startTimelineSound(event_, params, "settings.se");

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(1.2,
                                                [this]() noexcept
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
                                setWidgetText("BGM-text", bgm_enable_, u8"", u8"");
                                signalSettings();
                              });

    holder_ += event_.connect("SE:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                se_enable_ = !se_enable_;
                                setWidgetText("SE-text", se_enable_, u8"", u8"");
                                signalSettings();
                              });

    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "BGM");
    setupCommonTweens(event_, holder_, canvas_, "SE");

    applyDetail(detail);
    canvas_.startTween("start");
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

    setWidgetText("BGM-text", bgm_enable_, u8"", u8"");
    setWidgetText("SE-text",  se_enable_,  u8"", u8"");
  }

  void signalSettings() noexcept
  {
    Arguments args = {
      { "bgm-enable", bgm_enable_ },
      { "se-enable",  se_enable_ }
    };
    event_.signal("Settings:Changed", args);
  }

  void setWidgetText(const std::string& id, bool value, const std::string& true_text, const std::string& false_text) noexcept
  {
    const auto& widget = canvas_.at(id);
    widget->setParam("text", value ? true_text : false_text);
  }

};

}
