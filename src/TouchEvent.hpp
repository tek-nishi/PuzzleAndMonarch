#pragma once

//
// タッチ操作
//

#include <cinder/app/TouchEvent.h>
#include "Touch.hpp"
#include "Event.hpp"
#include "Arguments.hpp"


namespace ngs {

struct TouchEvent
{
  TouchEvent(Event<Arguments>& event)
    : event_(event)
  {
  }


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
      // イベント送信
      const auto& t = touches[0];
      Touch touch = {
        t.getId(),
        t.getPos(),
        t.getPrevPos()
      };
      Arguments arg = {
        { "touch", touch }
      };
      event_.signal("single_touch_began", arg);

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
      // イベント送信
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

      Arguments arg = {
        { "touches", touches_event }
      };
      event_.signal("multi_touch_moved", arg);
    }
    else if (first_touch_)
    {
      for (const auto& t : touches)
      {
        auto id = t.getId();
        if (id == first_touch_id_)
        {
          Touch touch = {
            id,
            t.getPos(),
            t.getPrevPos()
          };

          Arguments arg = {
            { "touch", touch }
          };
          event_.signal("single_touch_moved", arg);

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
          // イベント送信
          Touch touch = {
            t.getId(),
            t.getPos(),
            t.getPrevPos()
          };
          Arguments arg = {
            { "touch", touch }
          };
          event_.signal("single_touch_ended", arg);

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

  Event<Arguments>& event_;
};

}
