#pragma once

//
// 指定時間経過後に関数実行
// FIXME 関数ポインタ元のクラスが破棄されると未定義
// TODO 登録した関数ポインタを無効にする仕組み
// TODO 登録した関数ポインタを一気に実行する仕組み
//

#include <boost/noncopyable.hpp>


namespace ngs {

class CountExec
  : private boost::noncopyable
{
  struct Callback
    : private boost::noncopyable
  {
    Callback(double time_remain_, const std::function<void ()>& func_, bool forced)
      : time_remain(time_remain_),
        func(func_),
        force_count(forced)
    {}

    ~Callback() = default;


    double time_remain;
    std::function<void ()> func;
    bool force_count = false;
  };

  std::list<Callback> callbacks_;

  bool pause_ = false;


public:
  CountExec()  = default;
  ~CountExec() = default;


  void update(double delta_time) noexcept
  {
    // TIPS for文中でitを更新している
    for (auto it = std::begin(callbacks_); it != std::end(callbacks_); )
    {
      if (pause_ && !it->force_count)
      {
        ++it;
        continue;
      }

      it->time_remain -= delta_time;
      if (it->time_remain < 0.0)
      {
        it->func();
        // TIPS eraseは次のイテレーターを返す
        it = callbacks_.erase(it);
        continue;
      }
      ++it;
    }
  }


  void add(double time_remain, const std::function<void ()>& func, bool forced = false) noexcept
  {
    callbacks_.emplace_back(time_remain, func, forced);
  }

  void clear() noexcept
  {
    callbacks_.clear();
  }

  void pause(bool enable = false) noexcept
  {
    pause_ = enable;
  }

};

}
