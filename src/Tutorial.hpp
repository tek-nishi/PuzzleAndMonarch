#pragma once

//
// チュートリアル
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "EventSupport.hpp"


namespace ngs {

class Tutorial 
  : public Task
{

public:
  Tutorial(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common)
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("tutorial.canvas")),
              Params::load(params.getValueForKey<std::string>("tutorial.tweens"))),
      text_put_(params.getValueForKey<std::string>("tutorial.put")),
      text_blank_(params.getValueForKey<std::string>("tutorial.blank")),
      text_rotate_(params.getValueForKey<std::string>("tutorial.rotate"))
  {
    // ゲーム開始
    holder_ += event_.connect("Game:Start",
                              [this](const Connection&, const Arguments& args)
                              {
                                canvas_.startTween("start");
                              });
    
    // 長押しで置く
    holder_ += event_.connect("Game:panel:tap",
                              [this](const Connection&, const Arguments& args)
                              {
                                {
                                  const auto& pos = boost::any_cast<glm::vec3>(args.at("pos"));
                                  auto offset = canvas_.ndcToPos(pos);
                                  canvas_.setWidgetParam("blank", "offset", offset);
                                }
                              });

    holder_ += event_.connect("Game:blank:tap",
                              [this](const Connection&, const Arguments& args)
                              {
                                {
                                  const auto& pos = boost::any_cast<glm::vec3>(args.at("pos"));
                                  auto offset = canvas_.ndcToPos(pos);
                                  canvas_.setWidgetParam("blank", "offset", offset);
                                }
                              });
    // タップで回転
    // タップで移動
  }

  ~Tutorial() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    return active_;
  }


  
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  std::string text_put_;
  std::string text_blank_;
  std::string text_rotate_;

  bool active_ = true;
};

}
