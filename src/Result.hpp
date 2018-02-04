﻿#pragma once

//
// 結果画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"


namespace ngs {

class Result
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  int mode_ = 0;
  bool active_ = true;

  u_int frame_count = 0; 


public:
  // スコア
  struct Score
  {
    std::vector<int> scores;
    int total_score;
    int total_ranking;
  };


  Result(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
         const Score& score) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("result.canvas")),
              Params::load(params.getValueForKey<std::string>("result.tweens")))
  {
    count_exec_.add(2.0,
                    [this]() {
                      {
                        const auto& widget = canvas_.at("share");
                        widget->enable();
                      }
                      {
                        const auto& widget = canvas_.at("touch");
                        widget->enable();
                      }
                    });

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Agree." << std::endl;
                                canvas_.active(false);
                                count_exec_.add(0.5, [this]() noexcept
                                                     {
                                                       event_.signal("Result:Finished", Arguments());
                                                       active_ = false;
                                                     });
                              });
    holder_ += event_.connect("share:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Share." << std::endl;
                              });
    
    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "share");

    applyScore(score);
  }

  ~Result() = default;


private:
  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    switch (mode_)
    {
    case 0:
      {
      }
      break;
    }

#if 0
    frame_count += 1;

    if (!(frame_count % 8))
    {
      // 適当に値を設定
      const char* ids[] = {
        "score:1",
        "score:2",
        "score:3",
        "score:4",
        "score:5",
        "score:6",
        "score:7",
      };

      for (const auto& id : ids)
      {
        const auto& widget = canvas_.at(id);
        widget->setParam("text", std::to_string(ci::randInt(1000)));
      }

      {
        const auto& widget = canvas_.at("score:10");
        widget->setParam("text", std::to_string(ci::randInt(100000)));
      }

      {
        const char* ranking_text[] = {
          "Emperor",
          "King",
          "Viceroy",
          "Grand Duke",
          "Prince",
          "Landgrave",
          "Duke",
          "Marquess",
          "Margrave",
          "Count",  
          "Viscount",
          "Baron", 
          "Baronet",
        };

        const auto& widget = canvas_.at("score:11");
        widget->setParam("text", std::string(ranking_text[ci::randInt(12)]));
      }
    }
#endif


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
      const auto& widget = canvas_.at("score:10");
      widget->setParam("text", std::to_string(score.total_score));
    }
    {
      // TODO params.jsonで定義
      static const char* ranking_text[] =
      {
        "Emperor",
        "King",
        "Viceroy",
        "Grand Duke",
        "Prince",
        "Landgrave",
        "Duke",
        "Marquess",
        "Margrave",
        "Count",  
        "Viscount",
        "Baron", 
        "Baronet",
      };

      const auto& widget = canvas_.at("score:11");
      widget->setParam("text", std::string(ranking_text[score.total_ranking]));
    }
  }

};

}
