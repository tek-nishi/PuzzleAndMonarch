#pragma once

//
// ランキング
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "Share.h"
#include "Capture.h"


namespace ngs {

class Ranking
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  std::vector<std::string> ranking_text_;

  bool rank_in_  = false;
  u_int ranking_ = 0;
  size_t ranking_records_;

  // Share機能用の文章
  std::string share_text_;

  std::vector<std::string> rank_effects_;

  UI::Canvas canvas_;
  bool active_ = true;


public:
  Ranking(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common,
          const Arguments& args) noexcept
    : event_(event),
      ranking_text_(Json::getArray<std::string>(params["result.ranking"])),
      ranking_records_(params.getValueForKey<u_int>("game.ranking_records")),
      share_text_(params.getValueForKey<std::string>("ranking.share")),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("ranking.canvas")),
              Params::load(params.getValueForKey<std::string>("ranking.tweens")))
  {
    startTimelineSound(event_, params, "ranking.se");

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    if (args.count("rank_in"))
    {
      // Result画面の後にランクインした
      rank_in_ = boost::any_cast<bool>(args.at("rank_in"));
      ranking_ = boost::any_cast<u_int>(args.at("ranking"));

      canvas_.enableWidget("touch_back", false);
      canvas_.enableWidget("touch_agree");
    }

    auto records = args.count("records") && boost::any_cast<bool>(args.at("records"));
    if (args.count("view") && records)
    {
      // Title→Ranking で、ランクインした記録がある
      canvas_.enableWidget("view");

      // ボタンのレイアウト変更
      auto p = canvas_.getWidgetParam("view", "offset");
      glm::vec2 ofs = *(boost::any_cast<glm::vec2*>(p));
      ofs.x = -ofs.x;
      canvas_.setWidgetParam("touch", "offset", ofs);
    }

    holder_ += event_.connect("agree:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("root", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Ranking:Finished", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "Back to Title" << std::endl;
                              });

    holder_ += event_.connect("view:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("top10", "out-to-left");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("result", "in-from-right");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active(true);
                                                });

                                DOUT << "View start." << std::endl;
                              });
    
    holder_ += event_.connect("back:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("result", "out-to-right");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("top10", "in-from-left");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active(true);
                                                });

                                DOUT << "View end." << std::endl;
                              });

    holder_ += event_.connect("Ranking:UpdateScores",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                applyScore(args);
                                canvas_.startTween("update_score");
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

    // TOP10を閲覧
    if (!rank_in_)
    {
      for (int i = 0; i < ranking_records_; ++i)
      {
        char id[64];

        // Tween登録
        sprintf(id, "rank%d", i);
        setupCommonTweens(event_, holder_, canvas_, id, "rank");

        // コールバック登録
        sprintf(id, "rank%d:touch_ended", i);
        holder_ += event_.connect(id,
                                  [this, id, i](const Connection&, const Arguments&) noexcept
                                  {
                                    DOUT << i << " " << id << std::endl;
                                    // セーブデータがあれば読み込む
                                    Arguments args{
                                      { "rank", i }
                                    };
                                    event_.signal("Ranking:reload", args);
                                  });
      }
    }

    // ボタンイベント共通Tween
    setupCommonTweens(event_, holder_, canvas_, "agree");
    setupCommonTweens(event_, holder_, canvas_, "view");
    setupCommonTweens(event_, holder_, canvas_, "back");
    setupCommonTweens(event_, holder_, canvas_, "share");

    if (Share::canPost() && Capture::canExec())
    {
      // Share機能と画面キャプチャが有効ならUIも有効
      canvas_.enableWidget("share");

      // ボタンのレイアウト変更
      auto p = canvas_.getWidgetParam("share", "offset");
      glm::vec2 ofs = *(boost::any_cast<glm::vec2*>(p));
      ofs.x = -ofs.x;
      canvas_.setWidgetParam("back", "offset", ofs);
    }
    
    // NOTICE Title→Rankingの時は記録があるが、Result→Rankingの場合は記録が無い
    applyRankings(boost::any_cast<const ci::JsonTree&>(args.at("games")));

    canvas_.startCommonTween("root",
                             rank_in_ ? "in-from-left"
                                      : "in-from-right");
  }

  ~Ranking() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    if (rank_in_)
    {
      for (const auto& id : rank_effects_)
      {
        auto color = ci::hsvToRgb({ std::fmod(current_time * 2.0, 1.0), 1, 1 });
        canvas_.setWidgetParam(id, "color", color);
      }
    }

    return active_;
  }


  void applyRankings(const ci::JsonTree& rankings) noexcept
  {
    // NOTICE ソート済みの配列である事
    size_t num = std::min(rankings.getNumChildren(), ranking_records_);
    for (size_t i = 0; i < num; ++i)
    {
      const auto& json = rankings[i];
      {
        char id[16];
        std::sprintf(id, "%d", int(i + 1));
        auto score = json.getValueForKey<u_int>("score");
        canvas_.setWidgetText(id, std::to_string(score));
      }
      {
        char id[16];
        std::sprintf(id, "r%d", int(i + 1));
        auto rank = json.getValueForKey<u_int>("rank");
        canvas_.setWidgetText(id, ranking_text_[rank]);
      }
    }

    if (ranking_ < rankings.getNumChildren())
    {
      char id[16];

      std::sprintf(id, "%d", ranking_ + 1);
      rank_effects_.push_back(id);

      std::sprintf(id, "t%d", ranking_ + 1);
      rank_effects_.push_back(id);
      
      std::sprintf(id, "r%d", ranking_ + 1);
      rank_effects_.push_back(id);
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
      canvas_.setWidgetText(id, std::to_string(s));

      i += 1;
    }
      
    canvas_.setWidgetText("score:8",  std::to_string(boost::any_cast<u_int>(args.at("total_panels"))));
    canvas_.setWidgetText("score:9",  std::to_string(boost::any_cast<u_int>(args.at("total_score"))));
    canvas_.setWidgetText("score:10", ranking_text_[boost::any_cast<u_int>(args.at("total_ranking"))]);
  }
};

}
