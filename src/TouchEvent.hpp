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

    // 対角線上の位置を２つ目のタッチ位置とする
    auto center = ci::app::getWindowCenter();
    m_diagonal_prev_pos_ = center - pos + center;
  }

  void multiTouchMoved(const ci::app::MouseEvent& event) noexcept
  {
    glm::vec2 pos = event.getPos();

    std::vector<Touch> touches;
    touches.emplace_back(MOUSE_ID, false, pos, m_prev_pos_);

    if (event.isShiftDown())
    {
      // 平行移動
      auto d = pos - m_prev_pos_;
      auto pos2 = m_diagonal_prev_pos_ + d;
      touches.emplace_back(MULTI_ID, false, pos2, m_diagonal_prev_pos_);
      m_diagonal_prev_pos_ = pos2;
    }
    else
    {
      // 対角線上の位置を２つ目のタッチ位置とする
      auto center = ci::app::getWindowCenter();
      auto pos2 = center - pos + center;
      touches.emplace_back(MULTI_ID, false, pos2, m_diagonal_prev_pos_);
      m_diagonal_prev_pos_ = pos2;
    }

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
    // ID登録
    const auto& touches = event.getTouches();
    for (const auto& t : touches)
    {
      touch_id_.insert(t.getId());
    }

    if (touch_id_.size() == 1)
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
      DOUT << "single_touch_began" << std::endl;
    }
    else if (!multi_touch_)
    {
      // マルチタッチ開始
      event_.signal("multi_touch_began", Arguments());
      multi_touch_ = false;
      DOUT << "multi_touch_began" << std::endl;
    }

#if defined (CINDER_COCOA_TOUCH) && defined (DEBUG)
    if (touches.size() == 3)
    {
      DOUT << "triple" << std::endl;
      // 3本同時タップでキーボード
      event_.signal("App:show-keyboard", Arguments());
    }
#endif
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
    else if (touch_id_.size() == 1)
    {
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
      event_.signal("single_touch_moved", arg);
    }
  }
  
  void touchesEnded(const ci::app::TouchEvent& event) noexcept
  {
    if (touch_id_.empty()) return;

    auto num = touch_id_.size();

    const auto& touches = event.getTouches();
    if (touch_id_.size() == 1)
    {
      const auto& t = touches[0];
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
      DOUT << "single_touch_ended" << std::endl;
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
      multi_touch_ = false;
      event_.signal("multi_touch_ended", Arguments());
      DOUT << "multi_touch_ended" << std::endl;
    }
  }


private:
  enum {
    MOUSE_ID = 1,
    MULTI_ID
  };
  glm::vec2 m_prev_pos_;
  glm::vec2 m_diagonal_prev_pos_;

  std::set<uint32_t> touch_id_;
  bool multi_touch_ = false;

  Event<Arguments>& event_;
};

}
