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
#include "ConvertRank.hpp" 
#include "UISupport.hpp" 


namespace ngs {

class Result
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool rank_in_;
  u_int ranking_;
  bool high_score_;
  bool perfect_;

  std::vector<std::string> ranking_text_;

  int total_score_;
  int total_rank_;
  bool effect_ = false;
  double effect_speed_;

  ci::Anim<int> disp_score_;
  ci::Anim<int> disp_rank_;

  // Share機能用の文章
  std::string share_text_;

  ci::TimelineRef timeline_;

  UI::Canvas canvas_;
  bool active_ = true;



public:
  Result(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
         const Arguments& args) noexcept
    : event_(event),
      ranking_text_(Json::getArray<std::string>(params["result.ranking"])),
      effect_speed_(params.getValueForKey<double>("result.effect_speed")),
      timeline_(ci::Timeline::create()),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("result.canvas")),
              Params::load(params.getValueForKey<std::string>("result.tweens")))
  {
    startTimelineSound(event_, params, "result.se");

    rank_in_    = boost::any_cast<bool>(args.at("rank_in"));
    ranking_    = boost::any_cast<u_int>(args.at("ranking"));
    high_score_ = boost::any_cast<bool>(args.at("high_score"));

    const auto& score = boost::any_cast<const Score&>(args.at("score"));

    total_score_ = score.total_score;
    total_rank_  = score.total_ranking;
    perfect_     = score.perfect;

    share_text_ = replaceString(params.getValueForKey<std::string>("result.share"),
                                "%1",
                                std::to_string(score.total_score));

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event_.connect("agree:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Agree." << std::endl;
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  Arguments args {
                                                    { "rank_in", rank_in_ },
                                                    { "ranking", ranking_ },
                                                  };
                                                  event_.signal("Result:Finished", args);
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                              });
    
    holder_ += event_.connect("share:touch_ended",
                              [this, wipe_delay](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Share." << std::endl;

                                canvas_.active(false);
                                count_exec_.add(wipe_delay,
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

    // ランクイン時の演出
    auto disp_delay_2 = params.getValueForKey<float>("result.disp_delay_2");
    if (high_score_ || rank_in_)
    {
      count_exec_.add(disp_delay_2,
                      [this]() noexcept
                      {
                        effect_ = true;
                        if (high_score_)
                        {
                          canvas_.enableWidget("score:high-score");
                        }
                        else if (rank_in_)
                        {
                          canvas_.enableWidget("score:rank-in");
                        }

                        {
                          // SE
                          Arguments args{
                            { "name", std::string("rank-in") }
                          };
                          event_.signal("UI:sound", args);
                        }
                      });
    }
    if (perfect_)
    {
      count_exec_.add(disp_delay_2,
                      [this]() noexcept
                      {
                        effect_ = true;
                        canvas_.enableWidget("score:perfect");
                      });
    }

    if (Share::canPost() && Capture::canExec())
    {
      // Share機能と画面キャプチャが有効ならUIも有効
      canvas_.enableWidget("share");
      
      // ボタンのレイアウト変更
      auto p = canvas_.getWidgetParam("share", "offset");
      glm::vec2 ofs = *(boost::any_cast<glm::vec2*>(p));
      ofs.x = -ofs.x;
      canvas_.setWidgetParam("touch", "offset", ofs);
    }

    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "share");

    applyScore(score);
    tweenTotalScore(params);

    canvas_.startCommonTween("root", "in-from-left");

    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "touch", "touch:icon" },
      { "share", "share:icon" }
    };
    UI::startButtonTween(count_exec_, canvas_, 2.6, 0.2, widgets);
  }

  ~Result() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    timeline_->step(delta_time);

    if (effect_)
    {
      auto color = ci::hsvToRgb({ std::fmod(current_time * effect_speed_, 1.0), 0.75f, 1 });
      if (high_score_ || rank_in_)
      {
        canvas_.setWidgetParam("score:20", "color", color);
        canvas_.setWidgetParam("score:high-score", "color", color);
        canvas_.setWidgetParam("score:rank-in", "color", color);
      }
      if (perfect_)
      {
        canvas_.setWidgetParam("score:perfect", "color", color);
      }
    }

    return active_;
  }

  
  void applyScore(const Score& score) noexcept
  {
    panelScore(score.forest, "score:forest%d");
    panelScore(score.path, "score:path%d");

    // 街
    canvas_.setWidgetText("score:1",  std::to_string(score.scores[5]));
    // 教会
    canvas_.setWidgetText("score:2",  std::to_string(score.scores[6]));
    canvas_.setWidgetText("score:3",  std::to_string(score.total_panels));
  }

  void panelScore(const std::vector<u_int>& scores, const char* id_text)
  {
    if (scores.empty())
    {
      char id[16];
      sprintf(id, id_text, 0);
      canvas_.enableWidget(id);

      return;
    }

    int i = 0;
    float offset = 0.0f;
    for (auto f : scores)
    {
      if (i == 15) break;

      char id[16];
      sprintf(id, id_text, i);

      canvas_.setWidgetParam(id, "offset", glm::vec2(offset, 0));
      canvas_.enableWidget(id);
      auto s = std::to_string(f);
      canvas_.setWidgetText(id,  s);
      i += 1;
      offset += 6.0f + 5.0f * s.size();
    }
  }


  void tweenTotalScore(const ci::JsonTree& params) noexcept
  {
    {
      auto option = timeline_->apply(&disp_score_, 0, total_score_,
                                     params.getValueForKey<float>("result.disp_duration"),
                                     getEaseFunc(params.getValueForKey<std::string>("result.disp_ease")));
      option.delay(params.getValueForKey<float>("result.disp_delay"));
      option.updateFn([this]() noexcept
                      {
                        canvas_.setWidgetText("score:20", std::to_string(disp_score_));
                      });

      if (total_score_ > 0)
      {
        option.startFn([this]()
                       {
                         Arguments args{
                           { "name", std::string("drum-roll") }
                         };
                         event_.signal("UI:sound", args);
                       });
      }
    }
    {
      auto option = timeline_->apply(&disp_rank_, 0, total_rank_,
                                     params.getValueForKey<float>("result.disp_duration"),
                                     getEaseFunc(params.getValueForKey<std::string>("result.disp_ease")));
      option.delay(params.getValueForKey<float>("result.disp_delay"));
      option.updateFn([this]() noexcept
                      {
                        convertRankToText(disp_rank_, canvas_, "score:21", ranking_text_);
                      });
    }
  }

};

}
