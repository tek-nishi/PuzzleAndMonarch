#pragma once

//
// ゲーム本編UI
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"


namespace ngs {

class GameMain
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;
  ci::TimelineRef timeline_;

  std::vector<u_int> scores_;
  std::vector<ci::Color> scores_color_;
  

  bool active_ = true;


public:
  GameMain(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("gamemain.canvas")),
              Params::load(params.getValueForKey<std::string>("gamemain.tweens"))),
      timeline_(ci::Timeline::create())
  {
    DOUT << "GameMain::GameMain" << std::endl;
    startTimelineSound(event, params, "gamemain.se");
    
    // ゲーム開始演出 
    count_exec_.add(params.getValueForKey<double>("gamemain.start_delay"),
                    [this]() noexcept
                    {
                      event_.signal("Game:Start", Arguments());
                    });

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event_.connect("pause:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                // Pause開始
                                canvas_.active(false);
                                event_.signal("GameMain:pause", Arguments());
                                canvas_.startCommonTween("main", "out-to-right");

                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("pause_menu", "in-from-left");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active();
                                                });
                              });
    
    holder_ += event_.connect("resume:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                // Game続行(時間差で演出)
                                canvas_.active(false);
                                canvas_.startCommonTween("pause_menu", "out-to-left");

                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  canvas_.startCommonTween("main", "in-from-right");
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  canvas_.active();
                                                  event_.signal("GameMain:resume", Arguments());
                                                });
                              });
    
    holder_ += event_.connect("abort:touch_ended",
                              [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                              {
                                // ゲーム終了
                                canvas_.active(false);
                                canvas_.startCommonTween("pause_menu", "out-to-right");

                                count_exec_.add(wipe_delay,
                                                [this]() noexcept
                                                {
                                                  event_.signal("Game:Aborted", Arguments());
                                                });
                                count_exec_.add(wipe_duration,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                });
                                DOUT << "GameMain finished." << std::endl;
                              });

    // UI更新
    holder_ += event_.connect("Game:UI",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                char text[64];
                                auto remaining_time = boost::any_cast<double>(arg.at("remaining_time"));
                                if (remaining_time < 10.0)
                                {
                                  // 残り時間10秒切ったら焦らす
                                  sprintf(text, "0'%05.2f", remaining_time);
                                }
                                else
                                {
                                  int time = std::floor(remaining_time);
                                  int minutes = time / 60;
                                  int seconds = time % 60;
                                  sprintf(text, "%d'%02d", minutes, seconds);
                                }
                                canvas_.setWidgetText("time_remain", text);

                                // 時間が11秒切ったら色を変える
                                auto color = (remaining_time < 11.0) ? ci::Color(1, 0, 0)
                                                                     : ci::Color::white();
                                canvas_.setWidgetParam("time_remain",      "color", color);
                                canvas_.setWidgetParam("time_remain_icon", "color", color);
                              });

    holder_ += event_.connect("Game:UpdateScores",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                const auto& scores = boost::any_cast<std::vector<u_int>>(args.at("scores"));
                                updateScores(scores);
                              });


    // ゲーム完了
    auto end_delay = params.getValueForKey<double>("gamemain.end_delay");
    holder_ += event_.connect("Game:Finish",
                              [this, end_delay](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.active(false);
                                canvas_.startTween("end");
                                count_exec_.add(end_delay,
                                                [this]() noexcept
                                                {
                                                  active_ = false;
                                                  DOUT << "GameMain:end" << std::endl;
                                                });
                              });

    // パネル位置
    holder_ += event_.connect("Game:PutBegin",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                {
                                  const auto& pos = boost::any_cast<glm::vec3>(args.at("pos"));
                                  auto offset = canvas_.ndcToPos(pos);
                                  
                                  const auto& widget = canvas_.at("put_timer");
                                  widget->setParam("offset", offset);
                                  widget->enable();
                                }
                                canvas_.setWidgetParam("put_timer:body", "scale", glm::vec2());
                              });
    holder_ += event_.connect("Game:PutEnd",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.enableWidget("put_timer", false);
                              });
    holder_ += event_.connect("Game:PutHold",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                {
                                  const auto& pos = boost::any_cast<glm::vec3>(args.at("pos"));
                                  auto offset = canvas_.ndcToPos(pos);
                                  canvas_.setWidgetParam("put_timer", "offset", offset);
                                }
                                auto scale = boost::any_cast<float>(args.at("scale"));
                                auto alpha = getEaseFunc("OutExpo")(scale);
                                canvas_.setWidgetParam("put_timer:fringe", "alpha", alpha);
                                canvas_.setWidgetParam("put_timer:body", "scale", glm::vec2(scale));
                                canvas_.setWidgetParam("put_timer:body", "alpha", alpha);
                              });

    setupCommonTweens(event_, holder_, canvas_, "pause");
    setupCommonTweens(event_, holder_, canvas_, "resume");
    setupCommonTweens(event_, holder_, canvas_, "abort");

    setupScores(params);

    canvas_.startTween("start");
  }

  ~GameMain() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    timeline_->step(delta_time);

    return active_;
  }


  // 得点時の演出準備
  void setupScores(const ci::JsonTree& params) noexcept
  {
    const auto& score_rates = params["game.score_rates"];
    auto num = score_rates.getNumChildren();
    scores_.resize(num);
    std::fill(std::begin(scores_), std::end(scores_), u_int(0));

    scores_color_.resize(num);
    std::fill(std::begin(scores_color_), std::end(scores_color_), ci::Color::white());
  }

  void updateScores(const std::vector<u_int>& scores) noexcept
  {
    int i = 0;
    for (auto score : scores)
    {
      if (score != scores_[i])
      {
        scores_[i] = score;

        char id[16];
        std::sprintf(id, "score:%d", i + 1);
        canvas_.setWidgetParam(id, "text", std::to_string(score));

        timeline_->removeTarget(&scores_color_[i]);
        auto option = timeline_->applyPtr(&scores_color_[i],
                                          ci::Color(1, 0, 0), ci::Color::white(),
                                          0.8);
        option.updateFn([this, id, i]() noexcept
                        {
                          canvas_.setWidgetParam(id, "color", scores_color_[i]);
                        });
      }
      i += 1;
    }
  }

};

}
