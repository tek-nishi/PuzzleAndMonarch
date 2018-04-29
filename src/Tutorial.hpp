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
      direction_delay_(params.getValueForKey<double>("tutorial.direction_delay")),
      current_direction_delay_(direction_delay_),
      text_(Json::getArray<std::string>(params["tutorial.text"]))
  {
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

    holder_ += event_.connect("Game:PutPanel",
                              [this](const Connection&, const Arguments&)
                              {
                                if (disp_ && (disp_type_ == 2))
                                {
                                  disp_ = false;
                                  canvas_.startTween("end");
                                }

                                // パネルを設置した
                                operation_.insert(2);
                                current_direction_delay_ = direction_delay_;
                              });
    holder_ += event_.connect("Game:PanelRotate",
                              [this](const Connection&, const Arguments&)
                              {
                                if (disp_ && (disp_type_ == 1))
                                {
                                  disp_ = false;
                                  canvas_.startTween("end");
                                }

                                // パネルを回した
                                operation_.insert(1);
                                current_direction_delay_ = direction_delay_;
                              });
    holder_ += event_.connect("Game:PanelMove",
                              [this](const Connection&, const Arguments&)
                              {
                                if (disp_ && (disp_type_ == 0))
                                {
                                  disp_ = false;
                                  canvas_.startTween("end");
                                }

                                // パネルを移動した
                                operation_.insert(0);
                                current_direction_delay_ = direction_delay_;
                              });

    // 本編終了に伴い本タスクも終了
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments&)
                              {
                                active_ = false;
                              });

    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&)
                              {
                                active_ = false;
                              });
  }

  ~Tutorial() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    // 必須操作が無いと催促する感じ
    if (!disp_)
    {
      current_direction_delay_ -= delta_time;
      if (current_direction_delay_ < 0.0)
      {
        current_direction_delay_ = direction_delay_;
        bool disp = false;

        for (int i = 0; i < text_.size(); ++i)
        {
          if (!operation_.count(i))
          {
            canvas_.setWidgetText("blank:text", text_[i]);
            disp_type_ = i;
            disp = true;
            break;
          }
        }

        disp_ = disp;
        if (disp)
        {
          canvas_.startTween("start");
        }
        else
        {
          // すべて表示した
          active_ = false;
        }
      }
    }

    return active_;
  }

  
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  // 各種操作
  std::set<int> operation_; 

  double direction_delay_;
  double current_direction_delay_;

  bool disp_ = false;
  int disp_type_;

  std::vector<std::string> text_;

  bool active_ = true;
};

}
