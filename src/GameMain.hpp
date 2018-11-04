#pragma once

//
// ゲーム本編UI
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "UISupport.hpp"


namespace ngs {

class GameMain
  : public Task
{

public:
  GameMain(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common) noexcept
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("gamemain.canvas")),
              Params::load(params.getValueForKey<std::string>("gamemain.tweens"))),
      timeline_(ci::Timeline::create()),
      scores_(3, 0)
  {
    DOUT << "GameMain::GameMain" << std::endl;
    startTimelineSound(event, params, "gamemain.se");

    // ゲーム開始演出 
    count_exec_.add(params.getValueForKey<double>("gamemain.start_delay"),
                    [this]() noexcept
                    {
                      event_.signal("Game:Start", Arguments());
                      canvas_.active();
                    });

    auto wipe_delay    = params.getValueForKey<double>("ui.wipe.delay");
    auto wipe_duration = params.getValueForKey<double>("ui.wipe.duration");

    holder_ += event.connect("Tutorial:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                canvas_.enableWidget("time_remain_icon", false);
                                canvas_.enableWidget("time_remain",      false);
                                canvas_.enableWidget("scores",           false);
                              });

    holder_ += event.connect("pause:touch_ended",
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
                                                 startPauseTweens();
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 canvas_.active();
                                               });
                             });
    
    holder_ += event.connect("resume:touch_ended",
                             [this, wipe_delay, wipe_duration](const Connection&, const Arguments&) noexcept
                             {
                               // Game続行(時間差で演出)
                               canvas_.active(false);
                               canvas_.startCommonTween("pause_menu", "out-to-left");

                               count_exec_.add(wipe_delay,
                                               [this]() noexcept
                                               {
                                                 canvas_.startCommonTween("main", "in-from-right");
                                                 startPauseButtonTween(0.6);
                                               });
                               count_exec_.add(wipe_duration,
                                               [this]() noexcept
                                               {
                                                 canvas_.active();
                                                 event_.signal("GameMain:resume", Arguments());
                                               });
                             });
    
    holder_ += event.connect("abort:touch_ended",
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
    holder_ += event.connect("Game:UI",
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

    // ゲーム完了
    auto end_delay = params.getValueForKey<double>("gamemain.end_delay");
    holder_ += event.connect("Game:Finish",
                             [this, end_delay](const Connection&, const Arguments& args) noexcept
                             {
                               canvas_.active(false);
                                
                               auto delay = boost::any_cast<bool>(args.at("no_panels")) ? 2.0 : 0.0;
                               if (boost::any_cast<bool>(args.at("tutorial")))
                               {
                                 // チュートリアルの場合はGameMainを決して終了
                                 canvas_.startTween("tutorial-end");
                                 count_exec_.add(end_delay,
                                                 [this]() noexcept
                                                 {
                                                   active_ = false;
                                                   DOUT << "GameMain:end" << std::endl;
                                                 });
                               }
                               else
                               {
                                 // 終了演出
                                 count_exec_.add(delay,
                                                 [this, end_delay]()
                                                 {
                                                   canvas_.startTween("end");
                                                   count_exec_.add(end_delay,
                                                                   [this]() noexcept
                                                                   {
                                                                     active_ = false;
                                                                     DOUT << "GameMain:end" << std::endl;
                                                                   });
                                                 });
                               }
                             });

    // パネル位置
    holder_ += event.connect("Game:PutBegin",
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
    holder_ += event.connect("Game:PutEnd",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               canvas_.enableWidget("put_timer", false);
                             });
    holder_ += event.connect("Game:PutHold",
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
    // パネル設置
    holder_ += event.connect("Game:PutPanel",
                             [this](const Connection&, const Arguments& args)
                             {
                               auto panels = boost::any_cast<u_int>(args.at("remain_panel"));
                               updateScoreWidget(3, panels);
                             });

    // Like演出
    holder_ += event.connect("Game:convertPos",
                             [this](const Connection&, const Arguments& args)
                             {
                               DOUT << "Game:convertPos" << std::endl;
                               like_func_ = boost::any_cast<const std::function<glm::vec3 (const glm::ivec2&)>&>(args.at("callback"));
                             });
    holder_ += event.connect("Game:completed",
                             [this](const Connection&, const Arguments& args)
                             {
                               DOUT << "Game:completed" << std::endl;
                               const auto& positions = boost::any_cast<const std::vector<glm::ivec2>&>(args.at("positions"));
                                
                               static std::map<std::string, int> tbl{
                                 { "forests", 0 },
                                 { "path",    1 },
                                 { "church",  2 },
                               };
                               const auto* t = boost::any_cast<const char*>(args.at("type"));
                               int index = tbl.count(t) ? tbl.at(t) : -1;

                               // 演出開始
                               double delay = 0.2;
                               for (const auto& p : positions)
                               {
                                 count_exec_.add(delay,
                                                 [this, p, index]()
                                                 {
                                                   auto ndc_pos = like_func_(p);
                                                   auto ofs     = canvas_.ndcToPos(ndc_pos);

                                                   char id[16];
                                                   sprintf(id, "like%d", like_index_);
                                                   like_index_ = (like_index_ + 1) % 8;

                                                   canvas_.setTweenTarget(id, "like", 0);
                                                   canvas_.setWidgetParam(id, "offset", ofs);
                                                   canvas_.startTween("like");
                                                   Arguments se_args{
                                                     { "name", std::string("like") }
                                                   };
                                                   event_.signal("UI:sound", se_args);

                                                   if (index >= 0)
                                                   {
                                                     scores_[index] += 1;
                                                     updateScoreWidget(index, scores_[index]);
                                                   }
                                                 });
                                 delay += 0.1;
                               }
                             });

    setupCommonTweens(event_, holder_, canvas_, "pause");
    setupCommonTweens(event_, holder_, canvas_, "resume");
    setupCommonTweens(event_, holder_, canvas_, "abort");

    canvas_.active(false);
    canvas_.startTween("start");
    startPauseButtonTween(2.5);

    // like演出準備
    like_func_ = [](const glm::ivec2&)
                 {
                   return glm::vec3();
                 };
  }

  ~GameMain() = default;


private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);
    timeline_->step(delta_time);

    return active_;
  }

  void updateScoreWidget(int index, int score)
  {
    char id[16];
    std::sprintf(id, "score:%d", index);
    canvas_.setWidgetParam(id, "text", std::to_string(score));

    canvas_.setTweenTarget(id, "score", 0);
    canvas_.startTween("score");
  }


  void startPauseTweens()
  {
    // ボタン演出
    std::vector<std::pair<std::string, std::string>> widgets{
      { "abort",  "abort:icon" },
      { "resume", "resume:icon" },
    };
    UI::startButtonTween(count_exec_, canvas_, 0.53, 0.2, widgets);
  }

  void startPauseButtonTween(double delay)
  {
    std::vector<std::pair<std::string, std::string>> widgets{
      { "pause", "pause:icon" }
    };
    UI::startButtonTween(count_exec_, canvas_, delay, 0.0, widgets);
  }


  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;
  ci::TimelineRef timeline_;

  std::vector<u_int> scores_;

  bool active_ = true;
  bool tutorial_ = false;

  // いいね!!
  std::function<glm::vec3 (const glm::ivec2&)> like_func_;
  u_int like_index_ = 0;
};

}
