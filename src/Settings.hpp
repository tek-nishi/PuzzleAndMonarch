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

  bool bgm_enable_;
  bool se_enable_;
  bool has_records_;

  bool active_ = true;


public:
  // FIXME 受け渡しが冗長なのを解決したい
  struct Detail {
    bool bgm_enable;
    bool se_enable;
    bool has_records;
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

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event_.connect("agree:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  Arguments args = {
                                                    { "bgm-enable", bgm_enable_ },
                                                    { "se-enable",  se_enable_ }
                                                  };
                                                  event_.signal("Settings:Finished", args);
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Back to Title" << std::endl;
                              });

    holder_ += event_.connect("BGM:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                bgm_enable_ = !bgm_enable_;
                                canvas_.setWidgetText("BGM-text", bgm_enable_ ? u8"" : u8"");
                                signalSettings();
                              });

    holder_ += event_.connect("SE:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                se_enable_ = !se_enable_;
                                canvas_.setWidgetText("SE-text", se_enable_ ? u8"" : u8"");
                                signalSettings();
                              });

    // 記録削除画面へ
    holder_ += event_.connect("Trash:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("main", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("dust", "in-from-right");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active(true);
                                                });

                                DOUT << "Erase record." << std::endl;
                              });

    // 設定画面へ戻る
    holder_ += event_.connect("back:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("dust", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("main", "in-from-left");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active(true);
                                                });

                                DOUT << "Back to settings." << std::endl;
                              });

    // 記録を削除して設定画面へ戻る
    holder_ += event_.connect("erase-record:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                event_.signal("Settings:Trash", Arguments());
                                enableTrashButton(false);

                                canvas_.active(false);
                                canvas_.startCommonTween("dust", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("main", "in-from-left");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active(true);
                                                });

                                DOUT << "Erase record and back to settings." << std::endl;
                              });

    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "BGM");
    setupCommonTweens(event_, holder_, canvas_, "SE");
    setupCommonTweens(event_, holder_, canvas_, "Trash");
    setupCommonTweens(event_, holder_, canvas_, "back");
    setupCommonTweens(event_, holder_, canvas_, "erase-record");

    applyDetail(detail);
    canvas_.startCommonTween("root", "in-from-right");
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
    bgm_enable_  = detail.bgm_enable;
    se_enable_   = detail.se_enable;
    has_records_ = detail.has_records;

    canvas_.setWidgetText("BGM-text", bgm_enable_ ? u8"" : u8"");
    canvas_.setWidgetText("SE-text",  se_enable_  ? u8"" : u8"");

    enableTrashButton(has_records_);
  }

  void enableTrashButton(bool enable)
  {
    canvas_.enableWidget("Trash-text", enable);
    canvas_.enableWidget("Trash",      enable);
  }


  void signalSettings() noexcept
  {
    Arguments args{
      { "bgm-enable", bgm_enable_ },
      { "se-enable",  se_enable_ }
    };
    event_.signal("Settings:Changed", args);
  }
};

}
