#pragma once


namespace ngs {

class AutoRotateCamera
{

public:
  AutoRotateCamera(Event<Arguments>& event, const ci::JsonTree& params,
                   const std::function<void (float)>& camera_callback) noexcept
    : event_(event),
      camera_callback_(camera_callback),
      waiting_time_(params.getValueForKey<double>("auto_camera_delay")),
      speed_(params.getValueForKey<double>("auto_camera_rotation_speed")),
      delay_(waiting_time_)

  {
    holder_ += event_.connect("update",
                              std::bind(&AutoRotateCamera::update,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("single_touch_began",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;
                                
                                manipulating_ = true;
                              });
    holder_ += event_.connect("multi_touch_began",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                manipulating_ = true;
                              });

    holder_ += event_.connect("single_touch_ended",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                manipulating_ = false;
                                delay_  = 3;
                              });
    holder_ += event_.connect("multi_touch_ended",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                manipulating_ = false;
                                delay_  = 3;
                              });

    // 動作開始のきっかけ
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, Arguments&) noexcept
                              {
                                delay_ = waiting_time_;
                                active_ = true;
                              });
    holder_ += event_.connect("Ranking:begin",
                              [this](const Connection&, Arguments&) noexcept
                              {
                                delay_ = waiting_time_;
                                active_ = true;
                              });

    holder_ += event_.connect("Title:begin",
                              [this](const Connection&, Arguments&) noexcept
                              {
                                active_ = false;
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

    camera_callback_(delta_time * speed_);
  }


  Event<Arguments>& event_;
  ConnectionHolder holder_;

  std::function<void (float)> camera_callback_;

  double waiting_time_;
  double speed_;

  double delay_;

  // 操作中
  bool manipulating_ = false;

  // 動作ON/OFF
  bool active_ = false;
};

}
