#pragma once

//
// 実験用
//

#include <boost/noncopyable.hpp>
#include "ConnectionHolder.hpp"
#include "UIDrawer.hpp"
#include "TaskContainer.hpp"
#include "TweenCommon.hpp"


namespace ngs {

class Sandbox 
  : private boost::noncopyable
{
  void update(const Connection&, const Arguments& args) noexcept
  {
    auto current_time = boost::any_cast<double>(args.at("current_time"));
    auto delta_time   = boost::any_cast<double>(args.at("delta_time"));

    tasks_.update(current_time, delta_time);
  }


public:
  Sandbox(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : params_(params),
      event_(event),
      drawer_(params["ui"]),
      tween_common_(Params::load("tw_common.json"))
  {
    // system
    holder_ += event_.connect("update",
                              std::bind(&Sandbox::update,
                                        this, std::placeholders::_1, std::placeholders::_2));
  }

  ~Sandbox() = default;


private:
  // メンバ変数を最後尾で定義する実験
  const ci::JsonTree& params_;

  Event<Arguments>& event_;
  ConnectionHolder holder_;

  TaskContainer tasks_;

  // UI
  UI::Drawer drawer_;
  TweenCommon tween_common_;
};

}
