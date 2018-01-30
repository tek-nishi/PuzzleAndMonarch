#pragma once

//
// 結果画面
//

#include "Task.hpp"
#include "CountExec.hpp"
#include "UICanvas.hpp"


namespace ngs {

class Result
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  UI::Canvas canvas_;

  int mode_ = 0;
  bool active_ = true;

  u_int frame_count = 0; 


public:
  Result(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      canvas_(event, drawer, params["ui.camera"], Params::load(params.getValueForKey<std::string>("result.canvas")))
  {
    count_exec_.add(2.0,
                    [this]() {
                      {
                        const auto& widget = canvas_.at("share");
                        widget->enable();
                      }
                      {
                        const auto& widget = canvas_.at("touch");
                        widget->enable();
                      }
                    });

    holder_ += event_.connect("agree:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Agree." << std::endl;
                                canvas_.active(false);
                                count_exec_.add(0.5, [this]() noexcept
                                                     {
                                                       event_.signal("Result:Finished", Arguments());
                                                       active_ = false;
                                                     });
                              });
    holder_ += event_.connect("share:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                DOUT << "Share." << std::endl;
                              });
  }

  ~Result() = default;


  bool update(const double current_time, const double delta_time) noexcept override
  {
    count_exec_.update(delta_time);

    switch (mode_)
    {
    case 0:
      {
      }
      break;
    }

    frame_count += 1;

    if (!(frame_count % 8))
    {
      // 適当に値を設定
      const char* ids[] = {
        "score:1",
        "score:2",
        "score:3",
        "score:4",
        "score:5",
        "score:6",
        "score:7",
      };

      for (const auto& id : ids)
      {
        const auto& widget = canvas_.at(id);
        widget->setParam("text", std::to_string(ci::randInt(1000)));
      }

      {
        const auto& widget = canvas_.at("score:10");
        widget->setParam("text", std::to_string(ci::randInt(100000)));
      }

      {
        const char* ranking_text[] = {
          "Emperor",
          "King",
          "Viceroy",
          "Grand Duke",
          "Prince",
          "Landgrave",
          "Duke",
          "Marquess",
          "Margrave",
          "Count",  
          "Viscount",
          "Baron", 
          "Baronet",
        };

        const auto& widget = canvas_.at("score:11");
        widget->setParam("text", std::string(ranking_text[ci::randInt(12)]));
      }
    }



    return active_;
  }

  void draw(const glm::ivec2& window_size) noexcept override
  {
    canvas_.draw();
  }
};

}
