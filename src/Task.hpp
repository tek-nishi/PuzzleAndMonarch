﻿#pragma once

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
  virtual ~Task() = default;

  // 戻り値:false タスク終了
  virtual bool update(double current_time, double delta_time) noexcept = 0;
};

}
