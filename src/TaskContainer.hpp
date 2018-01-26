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
  TaskContainer() noexcept = default;
  ~TaskContainer() = default;


  void update() noexcept
  {
    auto result = std::remove_if(std::begin(tasks_), std::end(tasks_),
                                 [](std::unique_ptr<Task>& task) {
                                   return !task->update();
                                 });

    tasks_.erase(result, std::end(tasks_));
  }


  void draw(const glm::ivec2& window_size) noexcept
  {
    for (auto& task : tasks_)
    {
      task->draw(window_size);
    }
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
