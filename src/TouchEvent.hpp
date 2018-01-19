#pragma once

//
// タッチ操作
//

#include <cinder/app/TouchEvent.h>
#include "Touch.hpp"


namespace ngs {

struct TouchEvent
{
  // タッチされていない状況からの指一本タッチを「タッチ操作」と扱う
  void touchesBegan(const ci::app::TouchEvent& event)
  {
    bool first_touch = touch_id_.empty();

    const auto& touches = event.getTouches();
    for (const auto& t : touches)
    {
      touch_id_.push_back(t.getId());
    }

    if (first_touch)
    {
      first_touch_ = first_touch;
      const auto& t = touches[0];
      Touch touch = {
        t.getId(),
        t.getPos(),
        t.getPrevPos()
      };
      // シングルタッチ開始
    }
  }
  
  void touchesMoved(const ci::app::TouchEvent& event)
  {
    const auto& touches = event.getTouches();
    if (first_touch_)
    {
      for (const auto& t : touches)
      {
        if (std::find(std::begin(touch_id_), std::end(touch_id_), t.getId()) != std::end(touch_id_))
        {
          Touch touch = {
            t.getId(),
            t.getPos(),
            t.getPrevPos()
          };
          // シングルタッチが移動
        }
      }
    }
  }
  
  void touchesEnded(const ci::app::TouchEvent& event)
  {
    if (touch_id_.empty()) return;

    const auto& touches = event.getTouches();
    if (first_touch_)
    {
      auto id = touch_id_[0];
      for (const auto& t : touches)
      {
        if (t.getId() == id)
        {
          // シングルタッチのEnded操作
          Touch touch = {
            t.getId(),
            t.getPos(),
            t.getPrevPos()
          };

          first_touch_ = false;
          break;
        }
      }
    }

    // Touch情報を削除
    for (const auto& t : touches)
    {
      auto id = t.getId();
      auto it = std::find(std::begin(touch_id_), std::end(touch_id_), id);
      if (it != std::end(touch_id_))
      {
        touch_id_.erase(it);
      }
    }
  }


private:
  std::vector<uint32_t> touch_id_;
  bool first_touch_ = false;


};

}
