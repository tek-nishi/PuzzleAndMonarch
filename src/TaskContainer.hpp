#pragma once

//
// タスクコンテナ
//

#include "Task.hpp"
#include <list>
#include <algorithm>


namespace ngs {

class TaskContainer
  : private boost::noncopyable
{
  std::list<std::unique_ptr<Task>> tasks_;


public:
  TaskContainer()  = default;
  ~TaskContainer() = default;


  void update(const double current_time, const double delta_time) noexcept
  {
    for (auto it = std::begin(tasks_); it != std::end(tasks_); )
    {
      if (!(*it)->update(current_time, delta_time))
      {
        it = tasks_.erase(it);
        continue;
      }
      ++it;
    }
  }


  void draw(const glm::ivec2& window_size) noexcept
  {
    for (auto& task : tasks_)
    {
      task->draw(window_size);
    }
  }

  void clear() noexcept
  {
    tasks_.clear();
  }


  // 最前へ追加
  template <typename T, typename... Args>
  void pushFront(Args&&... args)
  {
    tasks_.push_front(std::make_unique<T>(args...));
  }

  // 最後尾へ追加
  template <typename T, typename... Args>
  void pushBack(Args&&... args)
  {
    tasks_.push_back(std::make_unique<T>(args...));
  }

};

}
