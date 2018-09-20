#pragma once

//
// チュートリアル
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"
#include "TweenUtil.hpp"
#include "EventSupport.hpp"
#include "CountExec.hpp"


namespace ngs {

class Tutorial 
  : public Task
{
  enum {
    PANEL_MOVE,
    PANEL_ROTATE,
    PANEL_PUT,

    GET_TOWN,
    GET_FOREST,
    GET_CHURCH,
  };


public:
  Tutorial(const ci::JsonTree& params, Event<Arguments>& event, Archive& archive, UI::Drawer& drawer, TweenCommon& tween_common)
    : event_(event),
      archive_(archive),
      canvas_(event, drawer, tween_common,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("tutorial.canvas")),
              Params::load(params.getValueForKey<std::string>("tutorial.tweens"))),
      direction_delay_(params.getValueForKey<double>("tutorial.direction_delay")),
      current_direction_delay_(direction_delay_),
      text_(Json::getArray<std::string>(params["tutorial.text"])),
      offset_(Json::getVecArray<glm::vec2>(params["tutorial.offset"]))
  {
    // TIPS コールバック関数にダミーを割り当てておく
    update_ = []() {};

    // Pause操作
    holder_ += event_.connect("GameMain:pause",
                              [this](const Connection&, const Arguments&)
                              {
                                pause_ = true;
                                // Pause中はチュートリアルの指示を消す
                                canvas_.startTween("pause");
                              });
    holder_ += event_.connect("GameMain:resume",
                              [this](const Connection&, const Arguments&)
                              {
                                pause_ = false;
                                canvas_.startTween("resume");
                              });

    // 各種操作
    struct
    {
      std::string key;
      std::string event;
      int type;
      bool like;
    }
    info[]
    {
      { "tutorial-put-panel",    "Game:PutPanel",    PANEL_PUT,    true },
      { "tutorial-rotate-panel", "Game:PanelRotate", PANEL_ROTATE, true },
      { "tutorial-move-panel",   "Game:PanelMove",   PANEL_MOVE,   true },

      { "tutorial-comp-path",   "Game:completed_path",    GET_TOWN,   false },
      { "tutorial-comp-forest", "Game:completed_forests", GET_FOREST, false },
      { "tutorial-comp-church", "Game:completed_church",  GET_CHURCH, false },
    };

    for (const auto& i : info)
    {
      if (!archive.getValue(i.key, false))
      {
        holder_ += event_.connect(i.event,
                                  [this, i](const Connection& connection, const Arguments&)
                                  {
                                    doneOperation(i.type, i.like);
                                    archive_.setRecord(i.key, true);

                                    connection.disconnect();
                                  });
      }
      else
      {
        doneOperation(i.type, false);
      }
    }

    // Callback登録
    holder_ += event_.connect("Tutorial:callback",
                              [this](const Connection&, const Arguments& args)
                              {
                                update_ = boost::any_cast<const std::function<void ()>&>(args.at("callback"));
                              });

    // 座標計算
    holder_ += event_.connect("Field:Positions",
                              [this](const Connection&, const Arguments& args)
                              {
                                static const char* label[] = {
                                  "blank",            // PANEL_MOVE
                                  "cursor",           // PANEL_ROTATE
                                  "cursor",           // PANEL_PUT

                                  "town",             // GET_TOWN
                                  "forest",           // GET_FOREST
                                  "church",           // GET_CHURCH
                                };

                                // 表示可能な情報を調査
                                for (int i = 0; i < text_.size(); ++i)
                                {
                                  if (args.count(label[i]))
                                  {
                                    waiting_.insert(i);
                                  }
                                  else
                                  {
                                    waiting_.erase(i);
                                  }
                                }

                                if (!disp_) return;

                                if (!args.count(label[disp_type_])) return;

                                const auto& pos = boost::any_cast<glm::vec3>(args.at(label[disp_type_]));
                                cur_ofs_ = canvas_.ndcToPos(pos) + offset_[disp_type_];
                                canvas_.setWidgetParam("blank", "offset", cur_ofs_);

                                // 置ける状況の場合だけ指示
                                if (disp_type_ == PANEL_PUT)
                                {
                                  auto can_put = boost::any_cast<bool>(args.at("can_put"));
                                  if (can_put != active_disp_)
                                  {
                                    if (can_put) canvas_.startTween("start");
                                    else         canvas_.startTween("end");

                                    active_disp_ = can_put;
                                  }
                                }
                              });

    // 本編終了に伴い本タスクも終了
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments&)
                              {
                                finishTask();
                              });

    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&)
                              {
                                finishTask();
                              });

    event_.signal("Tutorial:Begin", Arguments());
  }

  ~Tutorial() = default;
  

private:
  bool update(double current_time, double delta_time) noexcept override
  {
    if (pause_) return active_;
    
    // 更新関数を呼ぶ
    count_exec_.update(delta_time);
    update_();

    // 必須操作が無いと催促する感じ
    if (!disp_)
    {
      current_direction_delay_ -= delta_time;
      if (current_direction_delay_ < 0.0)
      {
        current_direction_delay_ = direction_delay_;

        // 未操作項目を探す
        bool disp = false;
        for (int i = 0; i < text_.size(); ++i)
        {
          if (!operation_.count(i) && waiting_.count(i))
          {
            canvas_.setWidgetText("blank:text", Localize::get(text_[i]));
            disp_type_ = i;
            disp = true;
            break;
          }
        }

        disp_ = disp;
        if (disp)
        {
          // 見つかった
          assert(disp_type_ < text_.size());
          canvas_.startTween("start");
        }
        else
        {
          if (operation_.size() < text_.size())
          {
            // まだ途中なので適当な間を置いて再チェック
            current_direction_delay_ = 1.0;
          }
        }
      }
    }

    return active_;
  }

  
  // タスク終了
  void finishTask()
  {
    active_ = false;
  }

  // 操作完了
  void doneOperation(int type, bool like)
  {
    if (disp_ && (disp_type_ == type))
    {
      // 指示を表示中なら完了演出を始める
      disp_ = false;
      canvas_.startTween("end");
      canvas_.setWidgetParam("like", "offset", cur_ofs_);
      if (like) canvas_.startTween("like");

      // if (!operation_.count(type))
      {
        // 最初に条件を満たした時は少し長めの待ち時間
        current_direction_delay_ = direction_delay_;
      }
    }
    operation_.insert(type);
  }
 
  
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  Archive& archive_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  // 各種操作
  std::set<int> operation_;
  std::set<int> waiting_;

  double direction_delay_;
  double current_direction_delay_;

  bool disp_ = false;
  bool active_disp_ = true;
  int disp_type_;
  glm::vec2 cur_ofs_;

  std::vector<std::string> text_;
  std::vector<glm::vec2> offset_;

  // 更新関数
  std::function<void ()> update_;

  bool pause_  = false;
  bool active_ = true;
};

}
