#pragma once

//
// 簡易タスク
// TODO タスクの登録
// TODO タスクのイテレート   
// TODO 削除 
//

#include <boost/noncopyable.hpp>


namespace ngs {

struct Task
  : private boost::noncopyable
{
  virtual ~Task() {} 

  // 戻り値:false タスク終了
  virtual bool update(const double current_time, const double delta_time) noexcept = 0;
  virtual void draw(const glm::ivec2& window_size) noexcept = 0;

};

}
