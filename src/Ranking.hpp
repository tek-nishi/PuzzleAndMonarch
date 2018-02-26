#pragma once

//
// ランキング
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "Score.hpp"


namespace ngs {

class Ranking
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  std::vector<std::string> ranking_text_;

  UI::Canvas canvas_;
  bool active_ = true;


public:
  Ranking(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      ranking_text_(Json::getArray<std::string>(params["result.ranking"])),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("ranking.canvas")),
              Params::load(params.getValueForKey<std::string>("ranking.tweens")))
  {
    // count_exec_.add(1.0,
    //                 [this]() {
    //                   const auto& widget = canvas_.at("touch");
    //                   widget->enable();
    //                 });

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                count_exec_.add(1.0, [this]() noexcept
                                                     {
                                                       event_.signal("Ranking:Finished", Arguments());
                                                       active_ = false;
                                                     });
                                DOUT << "Back to Title" << std::endl;
                              });

    holder_ += event_.connect("Ranking:UpdateScores",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                applyScore(args);
                                
                                const auto& widget = canvas_.at("scores");
                                widget->enable();
                                canvas_.startTween("update_score");

                                DOUT << "Ranking:UpdateScores" << std::endl;
                              });

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "agree");
    canvas_.startTween("start");
  }

  ~Ranking() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }


  void applyScore(const Arguments& args) noexcept
  {
    const auto& scores = boost::any_cast<const std::vector<u_int>&>(args.at("scores"));
    int i = 1;
    for (auto s : scores)
    {
      char id[16];
      std::sprintf(id, "score:%d", i);

      const auto& widget = canvas_.at(id);
      widget->setParam("text", std::to_string(s));

      i += 1;
    }
    {
      const auto& widget = canvas_.at("score:8");
      widget->setParam("text", std::to_string(boost::any_cast<u_int>(args.at("total_panels"))));
    }
    {
      const auto& widget = canvas_.at("score:9");
      widget->setParam("text", std::to_string(boost::any_cast<u_int>(args.at("total_score"))));
    }
    {
      const auto& widget = canvas_.at("score:10");
      widget->setParam("text", std::string(ranking_text_[boost::any_cast<u_int>(args.at("total_ranking"))]));
    }
  }
};

}
