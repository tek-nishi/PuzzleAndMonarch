﻿#pragma once

//
// UIの最上位
//

#include <map>
#include <string>
#include <boost/noncopyable.hpp>
#include <cinder/Timeline.h>
#include "UIWidgetsFactory.hpp"
#include "UIDrawer.hpp"
#include "Camera.hpp"
#include "TweenContainer.hpp"


namespace ngs { namespace UI {

class Canvas
  : private boost::noncopyable
{

public:
  Canvas(Event<Arguments>& event,
         UI::Drawer& drawer,
         TweenCommon& tween_common,
         const ci::JsonTree& camera_params,
         const ci::JsonTree& widgets_params,
         const ci::JsonTree& tween_params) noexcept
    : event_(event),
      drawer_(drawer),
      tween_common_(tween_common),
      camera_(camera_params),
      widgets_(widgets_factory_.construct(widgets_params)),
      timeline_(ci::Timeline::create()),
      tweens_(tween_params)
  {
    // FIXME near_zピッタリの位置だとmacOSのReleaseビルドで絵が出ない
    camera_.body().lookAt(glm::vec3(0, 0, camera_.getNearClip() + 0.001f), glm::vec3(0));

    makeQueryWidgets(widgets_);
    
    holder_ += event_.connect("resize", std::bind(&Canvas::resize, this,
                                                  std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("single_touch_began",
                              std::bind(&Canvas::touchBegan,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("single_touch_moved",
                              std::bind(&Canvas::touchMoved,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("single_touch_ended",
                              std::bind(&Canvas::touchEnded,
                                        this, std::placeholders::_1, std::placeholders::_2));

#if defined (DEBUG)
    holder_ += event_.connect("debug-info",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                debug_info_ = !debug_info_;
                              });
#endif
  }

  ~Canvas() = default;


  void resize(const Connection&, const Arguments&) noexcept
  {
    camera_.resize();
  }


  void update(const double delta_time) noexcept
  {
    timeline_->step(delta_time);
  }

  void draw() noexcept
  {
    ci::gl::enableDepth(false);
    ci::gl::disable(GL_CULL_FACE);
    ci::gl::enableAlphaBlending();

    auto& camera = camera_.body();
    ci::gl::setMatrices(camera);

    // TIPS CameraのNearClipでの四隅の座標を取得→描画領域
    glm::vec3 top_left;
    glm::vec3 top_right;
    glm::vec3 bottom_left;
    glm::vec3 bottom_right;
    camera.getNearClipCoordinates(&top_left, &top_right, &bottom_left, &bottom_right);
    ci::Rectf rect(top_left.x, bottom_right.y, bottom_right.x, top_left.y);

    widgets_->draw(rect, glm::vec2(1), drawer_);

#if defined (DEBUG)
    if (debug_info_)
    {
      widgets_->debugDraw();
    }
#endif
  }


  const UI::WidgetPtr& at(const std::string& name) const noexcept
  {
    return query_widgets_.at(name);
  }

  void active(const bool active = true) noexcept
  {
    active_ = active;
    
    touching_in_ = false;
    // FIXME 何度も実行して良いのか??
    touching_widget_.reset();
  }


  void startTween(const std::string& name) noexcept
  {
    const auto& contents = tweens_.at(name);
    for (const auto& c : contents)
    {
      const auto& widget = this->at(c.identifier);
      c.tween.set(timeline_, widget);
    }
  }

  void startCommonTween(const std::string& id, const std::string& name) noexcept
  {
    auto& tween = tween_common_.at(name);
    const auto& widget = this->at(id);
    tween.set(timeline_, widget);
  }


private:
  void makeQueryWidgets(const UI::WidgetPtr& widget) noexcept
  {
    if (widget->hasIdentifier())
    {
      query_widgets_.insert({ widget->getIdentifier(), widget });
      enumerated_widgets_.push_back(widget);
    }

    const auto& children = widget->getChildren();
    if (children.empty()) return;

    for (const auto& child : children)
    {
      makeQueryWidgets(child);
    }
  }


  // UI event
  void touchBegan(const Connection&, Arguments& arg) noexcept
  {
    if (!active_) return;

    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);

    // 列挙したWidgetをクリックしたか計算
    for (const auto& w : enumerated_widgets_)
    {
      if (!w->hasEvent()) continue;

      if (w->contains(pos))
      {
        // DOUT << "widget touch began: " << w->getIdentifier() << std::endl;
        std::string event = w->getEvent() + ":touch_began";
        Arguments args = {
          { "widget", w->getIdentifier() }
        };
        event_.signal(event, args);

        touching_widget_ = w;
        touching_in_ = true;
        touch.handled = true;
        break;
      }
    }
  }
  
  void touchMoved(const Connection&, Arguments& arg) noexcept
  {
    if (!active_) return;
    if (touching_widget_.expired()) return;

    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);
    touch.handled = true;

    auto widget = touching_widget_.lock();
    bool contains = widget->contains(pos);
    Arguments args = {
      { "widget", widget->getIdentifier() }
    };
    if (contains)
    {
      if (touching_in_)
      {
        // DOUT << "widget touch moved in-in: " << widget->getIdentifier() << std::endl;
      }
      else
      {
        // DOUT << "widget touch moved out-in: " << widget->getIdentifier() << std::endl;
        std::string event = widget->getEvent() + ":moved_in";
        event_.signal(event, args);
      }
    }
    else
    {
      if (touching_in_)
      {
        // DOUT << "widget touch moved in-out: " << widget->getIdentifier() << std::endl;
        std::string event = widget->getEvent() + ":moved_out";
        event_.signal(event, args);
      }
      else
      {
        // DOUT << "widget touch moved out-out: " << widget->getIdentifier() << std::endl;
      }
    }
    touching_in_ = contains;
  }


  void touchEnded(const Connection&, Arguments& arg) noexcept
  {
    if (!active_) return;
    if (touching_widget_.expired()) return;

    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);
    touch.handled = true;

    auto widget = touching_widget_.lock();
    if (widget->contains(pos))
    {
      // DOUT << "widget touch ended in: " << widget->getIdentifier() << std::endl;
      // イベント送信
      std::string event = widget->getEvent() + ":touch_ended";
      Arguments args = {
        { "widget", widget->getIdentifier() }
      };
      event_.signal(event, args);
    }
    else
    {
      // DOUT << "widget touch ended out: " << widget->getIdentifier() << std::endl;
    }

    touching_widget_.reset();
  }


  glm::vec2 calcUIPosition(const glm::vec2& pos) noexcept
  {
    const auto& camera = camera_.body();
    auto ray = camera.generateRay(pos, ci::app::getWindowSize());

    float t;
    ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 0, -1), &t);
    auto touch_pos = ray.calcPosition(t);
    // FIXME 暗黙の変換(vec3→vec2)
    glm::vec2 ui_pos = touch_pos;

    return ui_pos;
  }


  // メンバ変数の定義を最後に置く実験
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  Camera camera_;

  UI::WidgetsFactory widgets_factory_;
  UI::WidgetPtr widgets_; 

  // クエリ用
  std::map<std::string, UI::WidgetPtr> query_widgets_;
  std::vector<UI::WidgetPtr> enumerated_widgets_;
  
  std::weak_ptr<UI::Widget> touching_widget_;
  bool touching_in_ = false;

  UI::Drawer& drawer_;

  TweenCommon& tween_common_;

  bool active_ = true;

  ci::TimelineRef timeline_;
  TweenContainer tweens_;

#if defined (DEBUG)
  bool debug_info_ = false;
#endif

};


} }

