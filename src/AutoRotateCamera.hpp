#pragma once


namespace ngs {

class AutoRotateCamera
{

public:
  AutoRotateCamera(Event<Arguments>& event, const ci::JsonTree& params,
                   const std::function<void (float)>& camera_callback) noexcept
    : camera_callback_(camera_callback),
      waiting_time_(params.getValueForKey<double>("auto_camera_delay")),
      speed_(params.getValueForKey<double>("auto_camera_rotation_speed")),
      delay_(waiting_time_)

  {
    holder_ += event.connect("update",
                             std::bind(&AutoRotateCamera::update,
                                       this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event.connect("single_touch_began",
                             [this](const Connection&, Arguments& arg) noexcept
                             {
                               const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                               if (touch.handled) return;
                                
                               manipulating_  = true;
                               current_speed_ = 0.0;
                             });
    holder_ += event.connect("multi_touch_began",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               manipulating_  = true;
                               current_speed_ = 0.0;
                             });
    
    holder_ += event.connect("single_touch_ended",
                             [this](const Connection&, Arguments& arg) noexcept
                             {
                               const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                               if (touch.handled) return;

                               manipulating_ = false;
                               delay_        = 3;
                             });
    holder_ += event.connect("multi_touch_ended",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               manipulating_ = false;
                               delay_        = 3;
                             });

    // 動作開始のきっかけ
    holder_ += event.connect("Title:finished",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               active_        = false;
                               current_speed_ = 0.0;
                             });
    holder_ += event.connect("Game:Finish",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               delay_         = waiting_time_;
                               active_        = true;
                               current_speed_ = 0.0;
                             });
    holder_ += event.connect("Ranking:begin",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               if (!active_)
                               {
                                 delay_  = waiting_time_;
                                 active_ = true;
                               }
                             });

    holder_ += event.connect("App:ResignActive",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               // アプリがサスペンドした
                               if (active_)
                               {
                                 manipulating_  = true;
                                 current_speed_ = 0.0;
                               }
                             });
    holder_ += event.connect("App:BecomeActive",
                             [this](const Connection&, Arguments&) noexcept
                             {
                               // サスペンドから復帰
                               if (active_)
                               {
                                 manipulating_  = false;
                                 delay_         = 3;
                               }
                             });

  }

  ~AutoRotateCamera() = default;


private:
  // 一定時間操作されなかったら回転を始める 
  void update(const Connection&, Arguments& args) noexcept
  {
    if (!active_ || manipulating_) return;

    auto delta_time = boost::any_cast<double>(args.at("delta_time"));

    if (delay_ > 0.0)
    {
      delay_ -= delta_time;
      return;
    }

    // じわっと加速する
    current_speed_ += (speed_ - current_speed_) * (1 - std::pow(0.5, delta_time));
    camera_callback_(delta_time * current_speed_);
  }



  ConnectionHolder holder_;

  std::function<void (float)> camera_callback_;

  double waiting_time_;
  double speed_;

  double current_speed_ = 0.0;

  double delay_;

  // 操作中
  bool manipulating_ = false;

  // 動作ON/OFF
  bool active_ = false;
};

}
