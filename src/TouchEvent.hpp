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

    auto num = touch_id_.size();

    const auto& touches = event.getTouches();
    for (const auto& t : touches)
    {
      touch_id_.insert(t.getId());
    }

    if (first_touch)
    {
      const auto& t = touches[0];
      Touch touch = {
        t.getId(),
        t.getPos(),
        t.getPrevPos()
      };
      // シングルタッチ開始
      
      first_touch_    = first_touch;
      first_touch_id_ = t.getId();
    }

    if (num <= 1 && touch_id_.size() >= 2)
    {
      // マルチタッチ開始
    }
  }
  
  void touchesMoved(const ci::app::TouchEvent& event)
  {
    const auto& touches = event.getTouches();
    if (touches.size() > 1)
    {
      std::vector<Touch> touches_event;
      for (const auto& t : touches)
      {
        Touch touch = {
          t.getId(),
          t.getPos(),
          t.getPrevPos()
        };
        touches_event.push_back(touch);
      }
      // マルチタッチイベント

    }
    else if (first_touch_)
    {
      for (const auto& t : touches)
      {
        auto id = t.getId();
        if (touch_id_.count(id))
        {
          Touch touch = {
            id,
            t.getPos(),
            t.getPrevPos()
          };
          // シングルタッチが移動

          break;
        }
      }
    }
  }
  
  void touchesEnded(const ci::app::TouchEvent& event)
  {
    if (touch_id_.empty()) return;

    auto num = touch_id_.size();

    const auto& touches = event.getTouches();
    if (first_touch_)
    {
      for (const auto& t : touches)
      {
        if (t.getId() == first_touch_id_)
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
      touch_id_.erase(id);
    }
    
    if (num >= 2 && touch_id_.size() <= 1)
    {
      // マルチタッチ終了
    }
  }


private:
  std::set<uint32_t> touch_id_;

  bool first_touch_ = false;
  uint32_t first_touch_id_;

};

}
