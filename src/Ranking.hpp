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
  Ranking(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
          const ci::JsonTree& ranking) noexcept
    : event_(event),
      ranking_text_(Json::getArray<std::string>(params["result.ranking"])),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("ranking.canvas")),
              Params::load(params.getValueForKey<std::string>("ranking.tweens")))
  {
    startTimelineSound(event_, params, "ranking.se");

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(1.2,
                                                [this]() noexcept
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

    applyRankings(ranking);

    canvas_.startTween("start");
  }

  ~Ranking() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    return active_;
  }


  void applyRankings(const ci::JsonTree& rankings) noexcept
  {
    // NOTICE ソート済みで最大10個の配列である事
    size_t num = std::min(rankings.getNumChildren(), size_t(10));
    for (size_t i = 0; i < num; ++i)
    {
      const auto& json = rankings[i];

      {
        char id[16];
        std::sprintf(id, "%d", int(i + 1));
        auto score = json.getValueForKey<u_int>("score");
        UI::Canvas::setWidgetText(canvas_, id, std::to_string(score));
      }
      {
        char id[16];
        std::sprintf(id, "r%d", int(i + 1));
        auto rank = json.getValueForKey<u_int>("rank");
        UI::Canvas::setWidgetText(canvas_, id, ranking_text_[rank]);
      }
    }
  }

  void applyScore(const Arguments& args) noexcept
  {
    const auto& scores = boost::any_cast<const std::vector<u_int>&>(args.at("scores"));
    int i = 1;
    for (auto s : scores)
    {
      char id[16];
      std::sprintf(id, "score:%d", i);
      UI::Canvas::setWidgetText(canvas_, id, std::to_string(s));

      i += 1;
    }
      
    UI::Canvas::setWidgetText(canvas_, "score:8",  std::to_string(boost::any_cast<u_int>(args.at("total_panels"))));
    UI::Canvas::setWidgetText(canvas_, "score:9",  std::to_string(boost::any_cast<u_int>(args.at("total_score"))));
    UI::Canvas::setWidgetText(canvas_, "score:10", ranking_text_[boost::any_cast<u_int>(args.at("total_ranking"))]);
  }
};

}
