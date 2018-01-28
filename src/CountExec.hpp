#pragma once

//
// 指定時間経過後に関数実行
//

#include <boost/noncopyable.hpp>


namespace ngs {

class CountExec
  : private boost::noncopyable
{
  struct Callback
    : private boost::noncopyable
  {
    Callback(const double time_remain_, std::function<void ()> func_)
      : time_remain(time_remain_),
        func(func_)
    {}

    ~Callback() = default;


    double time_remain;
    std::function<void ()> func;
  };

  std::list<Callback> callbacks_;


public:
  void update(const double delta_time) noexcept
  {
    // TIPS for文中でitを更新している
    for (auto it = std::begin(callbacks_); it != std::end(callbacks_); )
    {
      it->time_remain -= delta_time;
      if (it->time_remain <= 0.0)
      {
        it->func();
        // TIPS eraseは次のイテレーターを返す
        it = callbacks_.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }


  void add(const double time_remain, std::function<void ()>& func) noexcept
  {
    callbacks_.emplace_back(time_remain, func);
  }

};

}
