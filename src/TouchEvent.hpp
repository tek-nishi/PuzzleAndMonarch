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
  : private boost::noncopyable
{
  TouchEvent(Event<Arguments>& event) noexcept
    : event_(event)
  {}


  // マウスでのタッチ操作代用
  void touchBegan(const ci::app::MouseEvent& event) noexcept
  {
    const auto& pos = event.getPos();
    Touch touch = {
      MOUSE_ID,
      false,
      pos,
      pos
    };
    Arguments arg = {
      { "touch", touch }
    };
    event_.signal("single_touch_began", arg);

    m_prev_pos_ = pos;
  }

  void touchMoved(const ci::app::MouseEvent& event) noexcept
  {
    const auto& pos = event.getPos();
    Touch touch = {
      MOUSE_ID,
      false,
      pos,
      m_prev_pos_
    };

    Arguments arg = {
      { "touch", touch }
    };
    event_.signal("single_touch_moved", arg);

    m_prev_pos_ = pos;
  }

  void touchEnded(const ci::app::MouseEvent& event) noexcept
  {
    const auto& pos = event.getPos();
    Touch touch = {
      MOUSE_ID,
      false,
      pos,
      m_prev_pos_
    };

    Arguments arg = {
      { "touch", touch }
    };
    event_.signal("single_touch_ended", arg);
  }

  // マウスでのマルチタッチ操作代用
  void multiTouchBegan(const ci::app::MouseEvent& event) noexcept
  {
    glm::vec2 pos = event.getPos();
    m_prev_pos_ = pos;
  }

  void multiTouchMoved(const ci::app::MouseEvent& event) noexcept
  {
    glm::vec2 pos = event.getPos();

    std::vector<Touch> touches;
    touches.emplace_back(MOUSE_ID, false, pos, m_prev_pos_);

    // 対角線上の位置を２つ目のタッチ位置とする
    auto center = ci::app::getWindowCenter();
    auto pos2      = center - pos + center;
    auto prev_pos2 = center - m_prev_pos_ + center;
    touches.emplace_back(MULTI_ID, false, pos2, prev_pos2);

    Arguments arg = {
      { "touches", touches }
    };
    event_.signal("multi_touch_moved", arg);

    m_prev_pos_ = pos;
  }

  void multiTouchEnded(const ci::app::MouseEvent& event) noexcept
  {
  }


  // タッチされていない状況からの指一本タッチを「タッチ操作」と扱う
  void touchesBegan(const ci::app::TouchEvent& event) noexcept
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
        false,
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
  
  void touchesMoved(const ci::app::TouchEvent& event) noexcept
  {
    const auto& touches = event.getTouches();
    if (touches.size() > 1)
    {
      // イベント送信
      std::vector<Touch> touches_event;
      for (const auto& t : touches)
      {
        touches_event.emplace_back(t.getId(), false, t.getPos(), t.getPrevPos());
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
            false,
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
  
  void touchesEnded(const ci::app::TouchEvent& event) noexcept
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
            false,
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
  enum {
    MOUSE_ID = 1,
    MULTI_ID
  };
  glm::vec2 m_prev_pos_;

  std::set<uint32_t> touch_id_;

  bool first_touch_ = false;
  uint32_t first_touch_id_;

  Event<Arguments>& event_;
};

}
