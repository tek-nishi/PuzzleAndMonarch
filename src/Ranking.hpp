#pragma once

//
// ランキング
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "ConvertRank.hpp" 
#include "UISupport.hpp"
#include "Share.h"
#include "Capture.h"


namespace ngs {

class Ranking
  : public Task
{
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
      rank_in_ = getValue<bool>(args, "rank_in");
      ranking_ = getValue<u_int>(args, "ranking");

      canvas_.enableWidget("touch_back", false);
      canvas_.enableWidget("touch_agree");
      canvas_.at("touch")->setSe("selected");
    }

    auto records = getValue(args, "records", false);
    if (records && args.count("view"))
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
                              [this, wipe_delay, wipe_duration, &params](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("top10", "out-to-left");
                                startTimelineSound(event_, params, "ranking.next-se");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  startSubTween();
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
                              [this, wipe_delay, wipe_duration, &params](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startCommonTween("result", "out-to-right");
                                startTimelineSound(event_, params, "ranking.back-se");
                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  startMainTween(rank_in_);
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

                                                  Share::post(AppText::get(share_text_), image,
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

    int rank_num = 0;
    if (!rank_in_)
    {
      // TOP10を閲覧
      applyRankingEffect(0);

      auto num = getValue<int>(args, "record_num");
      for (rank_num = 0; rank_num < num; ++rank_num)
      {
        char id[64];

        // Tween登録
        sprintf(id, "rank%d", rank_num);
        setupCommonTweens(event_, holder_, canvas_, id, "rank");

        // コールバック登録
        sprintf(id, "rank%d:touch_ended", rank_num);
        holder_ += event_.connect(id,
                                  [this, id, rank_num](const Connection&, const Arguments&) noexcept
                                  {
                                    DOUT << rank_num << " " << id << std::endl;
                                    // セーブデータがあれば読み込む
                                    Arguments args{
                                      { "rank", rank_num }
                                    };
                                    event_.signal("Ranking:reload", args);

                                    applyRankingEffect(rank_num);
                                  });
      }
    }
    // 不要なイベントを削除
    for (; rank_num < ranking_records_; ++rank_num)
    {
      char id[64];

      sprintf(id, "rank%d", rank_num);
      auto widget = canvas_.at(id);
      widget->active(false);
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
    startMainTween(rank_in_);
  }

  ~Ranking() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    if (!rank_effects_.empty())
    {
      auto color = ci::hsvToRgb({ std::fmod(current_time * 2.0, 1.0), 0.75f, 1 });
      for (const auto& id : rank_effects_)
      {
        canvas_.setWidgetParam(id, "color", color);
      }
      canvas_.setWidgetParam("perfect", "color", color);
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
        convertRankToText(rank, canvas_, id, ranking_text_);
      }
    }

    if (ranking_ < rankings.getNumChildren())
    {
      applyRankingEffect(ranking_);
    }
  }

  void applyRankingEffect(u_int ranking)
  {
    if (!rank_effects_.empty())
    {
      for (const auto& effect : rank_effects_)
      {
        canvas_.setWidgetParam(effect, "color", ci::Color::white());
      }
    }

    rank_effects_.clear();

    char id[16];

    std::sprintf(id, "%d", ranking + 1);
    rank_effects_.push_back(id);

    std::sprintf(id, "t%d", ranking + 1);
    rank_effects_.push_back(id);
      
    std::sprintf(id, "r%d", ranking + 1);
    rank_effects_.push_back(id);
  }


  void panelScore(const std::vector<std::vector<glm::ivec2>>* scores, const char* id_text)
  {
    // TODO 最大数チェック
    for (int i = 0; i < 15; ++i)
    {
      char id[16];
      sprintf(id, id_text, i);
      canvas_.enableWidget(id, false);
    }

    if (scores->empty())
    {
      char id[16];
      sprintf(id, id_text, 0);
      canvas_.enableWidget(id);
      canvas_.setWidgetText(id, std::string("0"));

      return;
    }

    int i = 0;
    float offset = 0.0f;
    for (auto& f : *scores)
    {
      if (i == 15) break;

      char id[16];
      sprintf(id, id_text, i);

      canvas_.setWidgetParam(id, "offset", glm::vec2(offset, 0));
      canvas_.enableWidget(id);
      auto s = std::to_string(f.size());
      canvas_.setWidgetText(id, s);
      ++i;
      offset += 4.5f + 3.0f * s.size();
    }
  }

  void applyScore(const Arguments& args)
  {
    {
      // 森の出来栄え
      auto* forest = boost::any_cast<std::vector<std::vector<glm::ivec2>>*>(args.at("completed_forest"));
      panelScore(forest, "score:forest%d");
    }
    {
      // 道の出来栄え
      auto* path = boost::any_cast<std::vector<std::vector<glm::ivec2>>*>(args.at("completed_path"));
      panelScore(path, "score:path%d");
    }

    const auto& scores = boost::any_cast<const std::vector<u_int>&>(args.at("scores"));
    // 教会
    canvas_.setWidgetText("score:7",  std::to_string(scores[6]));

    canvas_.setWidgetText("score:8", std::to_string(getValue<u_int>(args, "total_panels")));
    canvas_.setWidgetText("score:9", std::to_string(getValue<u_int>(args, "total_score")));

    auto rank = getValue<u_int>(args, "total_ranking");
    convertRankToText(rank, canvas_, "score:10", ranking_text_);

    // パーフェクト(Tutorialでは無視)
    bool perfect = getValue<bool>(args, "perfect") 
                   && !getValue<bool>(args, "tutorial");
    canvas_.enableWidget("perfect", perfect);
  }

  // メイン画面のボタン演出
  void startMainTween(bool rank_in)
  {
    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "touch", "touch_back" },
      { "view",  "view:icon" },
    };
    // NOTICE ゲーム本編→ランキングの場合は対象が違う
    if (rank_in)
    {
      widgets[0].second = "touch_agree";
    }

    UI::startButtonTween(count_exec_, canvas_, 0.55, 0.2, widgets);
  }

  // サブ画面のボタン演出
  void startSubTween()
  {
    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "back",  "back:icon" },
      { "share", "share:icon" },
    };
    UI::startButtonTween(count_exec_, canvas_, 0.55, 0.2, widgets);
  }



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
};

}
