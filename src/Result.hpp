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
#include "EventSupport.hpp"


namespace ngs {

class Result
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool rank_in_    = false;
  u_int ranking_   = 0;
  bool high_score_ = false;

  std::vector<std::string> ranking_text_;

  std::string share_text_;

  UI::Canvas canvas_;
  bool active_ = true;


public:
  Result(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
         const Arguments& args) noexcept
    : event_(event),
      ranking_text_(Json::getArray<std::string>(params["result.ranking"])),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("result.canvas")),
              Params::load(params.getValueForKey<std::string>("result.tweens")))
  {
    startTimelineSound(event_, params, "result.se");

    rank_in_ = boost::any_cast<bool>(args.at("rank_in"));
    ranking_ = boost::any_cast<u_int>(args.at("ranking"));
    
    const auto& score = boost::any_cast<const Score&>(args.at("score"));
    high_score_ = score.high_score;
    share_text_ = replaceString(params.getValueForKey<std::string>("result.share"),
                                "%1",
                                std::to_string(score.total_score));

    if (Share::canPost() && Capture::canExec())
    {
      // Share機能と画面キャプチャが有効ならUIも有効
      const auto& widget = canvas_.at("share");
      widget->enable();
    }

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Agree." << std::endl;
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(0.6,
                                                [this]() noexcept
                                                {
                                                  Arguments args {
                                                    { "rank_in", rank_in_ },
                                                    { "ranking", ranking_ },
                                                  };
                                                  event_.signal("Result:Finished", args);
                                                });
                                count_exec_.add(1.2,
                                                [this]() noexcept
                                                {
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
                                                  auto* image = Capture::execute();

                                                  event_.signal("App:pending-update", Arguments());

                                                  Share::post(share_text_, image,
                                                              [this](bool completed) noexcept
                                                              {
                                                                if (completed)
                                                                {
                                                                  // 記録につけとく
                                                                  DOUT << "Share: completed." << std::endl;
                                                                  event_.signal("Share:completed", Arguments());
                                                                }
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

      // const auto& widget = canvas_.at(id);
      // widget->setParam("text", std::to_string(s));
      UI::Canvas::setWidgetText(canvas_, id, std::to_string(s));

      i += 1;
    }

    UI::Canvas::setWidgetText(canvas_, "score:8",  std::to_string(score.total_panels));
    UI::Canvas::setWidgetText(canvas_, "score:9",  std::to_string(score.panel_turned_times));
    UI::Canvas::setWidgetText(canvas_, "score:10", std::to_string(score.panel_moved_times));

    UI::Canvas::setWidgetText(canvas_, "score:20", std::to_string(score.total_score));
    UI::Canvas::setWidgetText(canvas_, "score:21", std::string(ranking_text_[score.total_ranking]));
  }

};

}
