#pragma once

//
// Settings画面
//
#include "TweenUtil.hpp"
#include "UISupport.hpp"


namespace ngs {

class Settings
  : public Task
{

public:
  struct Condition
  {
    bool bgm_enable;
    bool se_enable;
    bool has_records;
    bool is_tutorial;
  };


  Settings(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
           const Condition& condition) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("settings.canvas")),
              Params::load(params.getValueForKey<std::string>("settings.tweens"))),
      sound_enable_(params.getValueForKey<std::string>("settings.sound_enable")),
      sound_disable_(params.getValueForKey<std::string>("settings.sound_disable"))
  {
    startTimelineSound(event, params, "settings.se");

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event.connect("agree:touch_ended",
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

    holder_ += event.connect("BGM:touch_ended",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               bgm_enable_ = !bgm_enable_;
                               setSoundButtonIcon("BGM:icon", bgm_enable_);
                               signalSettings();
                             });

    holder_ += event.connect("SE:touch_ended",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               se_enable_ = !se_enable_;
                               setSoundButtonIcon("SE:icon", se_enable_);
                               signalSettings();
                             });

    // 記録削除画面へ
    holder_ += event.connect("Trash:touch_ended",
                             [this, wipe_delay, wipe_duration, params](const Connection&, const Arguments&) noexcept
                             {
                               canvas_.active(false);
                               canvas_.startCommonTween("main", "out-to-left");
                               startTimelineSound(event_, params, "settings.next-se");
                               count_exec_.add(wipe_delay,
                                               [this]() noexcept
                                               {
                                                 canvas_.startCommonTween("dust", "in-from-right");
                                                 startSubTween();
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 canvas_.active(true);
                                               });

                               DOUT << "Erase record." << std::endl;
                             });

    // Tutorial開始
    holder_ += event.connect("Tutorial:touch_ended",
                             [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                             {
                               DOUT << "Begin tutorial" << std::endl;

                               canvas_.active(false);
                               canvas_.startCommonTween("root", "out-to-right");
                               count_exec_.add(wipe_delay,
                                               [this]() noexcept
                                               {
                                                 // FIXME かなり無理くり
                                                 Arguments args{
                                                   { "force-tutorial", true }
                                                 };
                                                 event_.signal("Title:finished", args);
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 active_ = false;
                                               });
                             });

    // 設定画面へ戻る
    holder_ += event.connect("back:touch_ended",
                             [this, wipe_delay, wipe_duration, &params](const Connection&, const Arguments&) noexcept
                             {
                               canvas_.active(false);
                               canvas_.startCommonTween("dust", "out-to-right");
                               startTimelineSound(event_, params, "settings.back-se");
                               count_exec_.add(wipe_delay,
                                               [this]() noexcept
                                               {
                                                 canvas_.startCommonTween("main", "in-from-left");
                                                 startMainTween();
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 canvas_.active(true);
                                               });

                               DOUT << "Back to settings." << std::endl;
                             });

    // 記録を削除して設定画面へ戻る
    holder_ += event.connect("erase-record:touch_ended",
                             [this, wipe_delay, wipe_duration, &params](const Connection&, const Arguments&) noexcept
                             {
                               event_.signal("Settings:Trash", Arguments());
                               disableTrashButton();

                               canvas_.active(false);
                               canvas_.startTween("erased");
                               canvas_.startCommonTween("dust", "out-to-right");
                               startTimelineSound(event_, params, "settings.back-se");
                               count_exec_.add(wipe_delay,
                                               [this]() noexcept
                                               {
                                                 canvas_.startCommonTween("main", "in-from-left");
                                                 startMainTween();
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 canvas_.active(true);
                                               });

                               DOUT << "Erase record and back to settings." << std::endl;
                             });

    setupCommonTweens(event, holder_, canvas_, "agree");
    setupCommonTweens(event, holder_, canvas_, "BGM");
    setupCommonTweens(event, holder_, canvas_, "SE");
    setupCommonTweens(event, holder_, canvas_, "Tutorial");
    setupCommonTweens(event, holder_, canvas_, "Trash");
    setupCommonTweens(event, holder_, canvas_, "back");
    setupCommonTweens(event, holder_, canvas_, "erase-record");

    applyCondition(condition);
    canvas_.startCommonTween("root", "in-from-right");
    startMainTween();
  }

  ~Settings() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }


  // 各種初期状態を決める
  void applyCondition(const Condition& condition) noexcept
  {
    bgm_enable_ = condition.bgm_enable;
    se_enable_  = condition.se_enable;

    // BGMとSE
    setSoundButtonIcon("BGM:icon", bgm_enable_);
    setSoundButtonIcon("SE:icon",  se_enable_);

    // Tutorial
    if (condition.is_tutorial)
    {
      setupForTutorial();
    }
    // 記録の削除
    if (!condition.has_records)
    {
      disableTrashButton();
    }
  }


  void setupForTutorial()
  {
    // ボタン無効
    canvas_.enableWidget("Tutorial-text", false);
    canvas_.enableWidget("Tutorial",      false);

    changeSoundButtonsOffset();
  }

  void changeSoundButtonsOffset()
  {
    const char* tbl[]{
      "BGM-text",
      "BGM",
      "SE-text",
      "SE"
    };

    for (const auto* id : tbl)
    {
      canvas_.setWidgetParam(id, "offset", glm::vec2());
    }
  }

  void disableTrashButton()
  {
    canvas_.enableWidget("Trash-text", false);
    canvas_.enableWidget("Trash",      false);
  }

  void setSoundButtonIcon(const std::string& id, bool enable)
  {
    canvas_.setWidgetText(id, enable ? sound_enable_ : sound_disable_);
  }

  void signalSettings() noexcept
  {
    Arguments args{
      { "bgm-enable", bgm_enable_ },
      { "se-enable",  se_enable_ }
    };
    event_.signal("Settings:Changed", args);
  }

  // メイン画面のボタン演出
  void startMainTween()
  {
    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "BGM",      "BGM:icon" },
      { "SE",       "SE:icon" },
      { "Tutorial", "Tutorial:icon" },
      { "Trash",    "Trash:icon" },
      { "touch",    "touch:icon" },
    };
    UI::startButtonTween(count_exec_, canvas_, 0.53, 0.15, widgets);
  }

  // サブ画面のボタン演出
  void startSubTween()
  {
    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "back",         "back:icon" },
      { "erase-record", "erase-record:icon" },
    };
    UI::startButtonTween(count_exec_, canvas_, 0.53, 0.15, widgets);
  }



  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  bool bgm_enable_;
  bool se_enable_;

  std::string sound_enable_;
  std::string sound_disable_;

  bool active_ = true;
};

}
