#pragma once

//
// 一定時間コールバックを実行する
//

#include <boost/noncopyable.hpp>


namespace ngs {

class FixedTimeExec
  : private boost::noncopyable
{
  struct Callback
    : private boost::noncopyable
  {
    Callback(double delay_time_, double time_remain_, const std::function<bool (double delta_time)>& func_)
      : delay_time(delay_time_),
        time_remain(time_remain_),
        infinit(time_remain_ < 0.0),
        func(func_)
    {}

    ~Callback() = default;

    double delay_time;
    double time_remain;
    bool infinit;

    std::function<bool (double)> func;
  };

  std::list<Callback> callbacks_;


public:
  FixedTimeExec()  = default;
  ~FixedTimeExec() = default;

  
  void update(double delta_time) noexcept
  {
    // TIPS for文中でitを更新している
    for (auto it = std::begin(callbacks_); it != std::end(callbacks_); )
    {
      if (it->delay_time <= 0.0)
      {
        auto result = it->func(delta_time);
        if (result && it->infinit)
        {
          ++it;
          continue;
        }

        it->time_remain -= delta_time;
        if (it->time_remain <= 0.0)
        {
          // TIPS eraseは次のイテレーターを返す
          it = callbacks_.erase(it);
          continue;
        }
      }
      it->delay_time -= delta_time;
      ++it;
    }
  }


  void add(double delay_time, double time_remain, const std::function<bool (double)>& func) noexcept
  {
    callbacks_.emplace_back(delay_time, time_remain, func);
  }

  void clear() noexcept
  {
    callbacks_.clear();
  }
};

}
