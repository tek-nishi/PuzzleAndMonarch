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
  enum {
    PANEL_MOVE,
    PANEL_ROTATE,
    PANEL_PUT,

    GET_FOREST,
    GET_TOWN,
    GET_CHURCH,
  };


public:
  Tutorial(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer, TweenCommon& tween_common)
    : event_(event),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("tutorial.canvas")),
              Params::load(params.getValueForKey<std::string>("tutorial.tweens"))),
      direction_delay_(params.getValueForKey<double>("tutorial.direction_delay")),
      current_direction_delay_(direction_delay_),
      text_(Json::getArray<std::string>(params["tutorial.text"])),
      offset_(Json::getVecArray<glm::vec2>(params["tutorial.offset"]))
  {
    // Pause操作
    holder_ += event_.connect("GameMain:pause",
                              [this](const Connection&, const Arguments&)
                              {
                                pause_ = true;

                                // Pause中はチュートリアルの指示を消す
                                if (disp_)
                                {
                                  canvas_.startTween("pause");
                                }
                              });
    holder_ += event_.connect("GameMain:resume",
                              [this](const Connection&, const Arguments&)
                              {
                                pause_ = false;

                                if (disp_)
                                {
                                  canvas_.startTween("start");
                                }
                              });

    // 各種操作
    holder_ += event_.connect("Game:PutPanel",
                              [this](const Connection&, const Arguments&)
                              {
                                doneOperation(PANEL_PUT);
                              });
    holder_ += event_.connect("Game:PanelRotate",
                              [this](const Connection&, const Arguments&)
                              {
                                doneOperation(PANEL_ROTATE);
                              });
    holder_ += event_.connect("Game:PanelMove",
                              [this](const Connection&, const Arguments&)
                              {
                                doneOperation(PANEL_MOVE);
                              });
    holder_ += event_.connect("Game:completed_forests",
                              [this](const Connection&, const Arguments&)
                              {
                                doneOperation(GET_FOREST);
                              });
    holder_ += event_.connect("Game:completed_path",
                              [this](const Connection&, const Arguments&)
                              {
                                doneOperation(GET_TOWN);
                              });
    holder_ += event_.connect("Game:completed_church",
                              [this](const Connection&, const Arguments&)
                              {
                                doneOperation(GET_CHURCH);
                              });

    // 座標計算
    holder_ += event_.connect("Field:Positions",
                              [this](const Connection&, const Arguments& args)
                              {
                                if (!disp_) return;

                                static const char* label[] = {
                                  "blank",            // PANEL_MOVE
                                  "cursor",           // PANEL_ROTATE
                                  "cursor",           // PANEL_PUT

                                  "forest",           // GET_FOREST
                                  "town",             // GET_TOWN
                                  "church",           // GET_CHURCH
                                };

                                if (!args.count(label[disp_type_])) return;

                                const auto& pos = boost::any_cast<glm::vec3>(args.at(label[disp_type_]));
                                auto offset = canvas_.ndcToPos(pos) + offset_[disp_type_];
                                canvas_.setWidgetParam("blank", "offset", offset);
                              });

    // 本編終了に伴い本タスクも終了
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments&)
                              {
                                event_.signal("Tutorial:Finish", Arguments());
                                active_ = false;
                              });

    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&)
                              {
                                event_.signal("Tutorial:Finish", Arguments());
                                active_ = false;
                              });

    event_.signal("Tutorial:Begin", Arguments());
  }

  ~Tutorial() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    if (pause_) return active_;

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
          assert(disp_type_ < text_.size());
          canvas_.startTween("start");
        }
        else
        {
          // すべて表示した
          DOUT << "Tutorial:Finish" << std::endl;
          event_.signal("Tutorial:Finish", Arguments());
          active_ = false;
        }
      }
    }

    return active_;
  }


  // 操作完了
  void doneOperation(int type)
  {
    if (disp_ && (disp_type_ == type))
    {
      disp_ = false;
      canvas_.startTween("end");
    }

    // パネルを設置した
    operation_.insert(type);
    current_direction_delay_ = direction_delay_;
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

  glm::vec3 disp_pos_;

  std::vector<std::string> text_;
  std::vector<glm::vec2> offset_;

  bool pause_  = false;
  bool active_ = true;
};

}
