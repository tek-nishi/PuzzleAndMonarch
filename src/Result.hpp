#pragma once

//
// 結果画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "Score.hpp"
#include "Share.h"
#include "Capture.h"


namespace ngs {

class Result
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool high_score_;

  std::vector<std::string> ranking_text_;

  std::string share_text_;

  UI::Canvas canvas_;
  bool active_ = true;


public:
  Result(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
         const Score& score) noexcept
    : event_(event),
      high_score_(score.high_score),
      ranking_text_(Json::getArray<std::string>(params["result.ranking"])),
      share_text_(replaceString(params.getValueForKey<std::string>("result.share"),
                                "%1",
                                std::to_string(score.total_score))),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("result.canvas")),
              Params::load(params.getValueForKey<std::string>("result.tweens")))
  {
    {
      // サウンド再生
      Arguments args{
        { "timeline", params["result.se"] }
      };
      event_.signal("SE:timeline", args);
    }

    count_exec_.add(2.0,
                    [this]() {
                      if (Share::canPost() && Capture::canExec())
                      {
                        // Share機能と画面キャプチャが有効じゃないと使えない
                        const auto& widget = canvas_.at("share");
                        widget->enable();
                      }
                      // {
                      //   const auto& widget = canvas_.at("touch");
                      //   widget->enable();
                      // }
                    });

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Agree." << std::endl;
                                canvas_.active(false);
                                count_exec_.add(0.5,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Result:Finished", Arguments());
                                                  active_ = false;
                                                });
                              });
    
    holder_ += event_.connect("share:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Share." << std::endl;

                                canvas_.active(false);
                                count_exec_.add(0.5,
                                                [this]() noexcept
                                                {
                                                  event_.signal("App:pending-update", Arguments());

                                                  Share::post(share_text_, Capture::execute(),
                                                              [this]() noexcept
                                                              {
                                                                event_.signal("App:resume-update", Arguments());
                                                                canvas_.active(true);
                                                              });
                                                });
                              });
    
    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "share");

    applyScore(score);
    canvas_.startTween("start");
  }

  ~Result() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    if (high_score_)
    {
      const auto& widget = canvas_.at("score:10");
      auto color = ci::hsvToRgb({ std::fmod(current_time * 2.0, 1.0), 1, 1 });
      widget->setParam("color", color);
    }

    return active_;
  }

  
  void applyScore(const Score& score) noexcept
  {
    int i = 1;
    for (auto s : score.scores)
    {
      char id[16];
      std::sprintf(id, "score:%d", i);

      const auto& widget = canvas_.at(id);
      widget->setParam("text", std::to_string(s));

      i += 1;
    }
    {
      const auto& widget = canvas_.at("score:8");
      widget->setParam("text", std::to_string(score.total_panels));
    }
    {
      const auto& widget = canvas_.at("score:10");
      widget->setParam("text", std::to_string(score.total_score));
    }
    {
      const auto& widget = canvas_.at("score:11");
      widget->setParam("text", std::string(ranking_text_[score.total_ranking]));
    }
  }

};

}
