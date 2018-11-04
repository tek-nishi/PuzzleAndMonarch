#pragma once

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
    camera_.body().lookAt(glm::vec3(0, 0, camera_.getNearClip() + 0.001f), glm::vec3());

    makeQueryWidgets(widgets_);

    holder_ += event_.connect("single_touch_began", 0,
                              std::bind(&Canvas::touchBegan,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("single_touch_moved", 0,
                              std::bind(&Canvas::touchMoved,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("single_touch_ended", 0,
                              std::bind(&Canvas::touchEnded,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("multi_touch_began", 0,
                              std::bind(&Canvas::multiTouchBegan,
                                        this, std::placeholders::_1, std::placeholders::_2));

    // system
    holder_ += event_.connect("resize",
                              std::bind(&Canvas::resize,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("update",
                              std::bind(&Canvas::update,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("draw", 0,
                              std::bind(&Canvas::draw,
                                        this, std::placeholders::_1, std::placeholders::_2));


#if defined (DEBUG)
    holder_ += event_.connect("debug-info",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                debug_info_ = !debug_info_;
                              });
    
    holder_ += event_.connect("debug-canvas-draw",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                debug_draw_ = !debug_draw_;
                              });
#endif
  }

  ~Canvas() = default;


  const UI::WidgetPtr& at(const std::string& name) const noexcept
  {
    return query_widgets_.at(name);
  }

  bool isExists(const std::string& name) const noexcept
  {
    return query_widgets_.count(name);
  }


  void active(const bool active = true) noexcept
  {
    active_ = active;
    
    touching_in_ = false;
    // FIXME 何度も実行して良いのか??
    first_touched_widget_.reset();
    touching_widget_.reset();
  }


  void startTween(const std::string& name)
  {
    const auto& contents = tweens_.at(name);
    for (const auto& c : contents)
    {
      const auto& widget = this->at(c.identifier);
      c.tween.set(timeline_, widget);
    }
  }

  void stopTween(const std::string& name)
  {
    auto& contents = tweens_.at(name);
    for (const auto& c : contents)
    {
      const auto& widget = this->at(c.identifier);
      c.tween.stop(timeline_, widget);
    }
  }

  void setTweenTarget(const std::string& id, const std::string& name, size_t index)
  {
    auto& contents = tweens_.at(name);
    contents[index].identifier = id;
  }


  void startCommonTween(const std::string& id, const std::string& name)
  {
    auto& tween = tween_common_.at(name);
    const auto& widget = this->at(id);
    tween.set(timeline_, widget);
  }

  glm::vec2 ndcToPos(const glm::vec3& pos) const noexcept
  {
    glm::vec3 top_left;
    glm::vec3 top_right;
    glm::vec3 bottom_left;
    glm::vec3 bottom_right;

    camera_.body().getNearClipCoordinates(&top_left, &top_right,
                                          &bottom_left, &bottom_right);

    return { top_left.x + (top_right.x - top_left.x) * (pos.x + 1.0f) * 0.5f,
             bottom_left.y + (top_left.y - bottom_left.y) * (pos.y + 1.0f) * 0.5f };
  }


  // 書き換え補助関数
  void setWidgetText(const std::string& id, const std::string& text) noexcept
  {
#if defined DEBUG
    if (!this->isExists(id))
    {
      DOUT << "No widget: " << id << std::endl;
      return;
    }
#endif

    const auto& widget = this->at(id);
    widget->setParam("text", text);
  }

  bool isEnableWidget(const std::string& id) noexcept
  {
#if defined DEBUG
    if (!this->isExists(id))
    {
      DOUT << "No widget: " << id << std::endl;
      return false;
    }
#endif

    const auto& widget = this->at(id);
    return widget->isEnable();
  }

  void enableWidget(const std::string& id, bool enable = true) noexcept
  {
#if defined DEBUG
    if (!this->isExists(id))
    {
      DOUT << "No widget: " << id << std::endl;
      return;
    }
#endif

    const auto& widget = this->at(id);
    widget->enable(enable);
  }

  void activeWidget(const std::string& id, bool active = true) noexcept
  {
#if defined DEBUG
    if (!this->isExists(id))
    {
      DOUT << "No widget: " << id << std::endl;
      return;
    }
#endif

    const auto& widget = this->at(id);
    if (!active && widget->hasEvent())
    {
      auto w = touching_widget_.lock();
      if (w == widget)
      {
        // 中断
        DOUT << "widget touch abort." << std::endl;
        signalEventMessage(widget, ":cancel");
        touching_widget_.reset();
      }
    }
    widget->active(active);
  }

  template <typename T>
  void setWidgetParam(const std::string& id, const std::string& param_id, const T& param) noexcept
  {
#if defined DEBUG
    if (!this->isExists(id))
    {
      DOUT << "No widget: " << id << std::endl;
      return;
    }
#endif

    const auto& widget = this->at(id);
    widget->setParam(param_id, param);
  }

  boost::any getWidgetParam(const std::string& id, const std::string& param_id) noexcept
  {
#if defined DEBUG
    if (!this->isExists(id))
    {
      DOUT << "No widget: " << id << std::endl;
      return { };
    }
#endif

    const auto& widget = this->at(id);
    return widget->getParam(param_id);
  }


  // 何かしらTween中か？
  bool hasTween() const noexcept
  {
    return !timeline_->empty();
  }


private:
  void resize(const Connection&, const Arguments&) noexcept
  {
    camera_.resize();
  }

  void update(const Connection&, const Arguments& args) noexcept
  {
    auto delta_time = boost::any_cast<double>(args.at("delta_time"));
    timeline_->step(delta_time);
  }

  void draw(const Connection&, const Arguments&) noexcept
  {
#if defined (DEBUG)
    if (debug_draw_) return;
#endif
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

    widgets_->draw(rect, drawer_, 1.0f);

#if defined (DEBUG)
    if (debug_info_)
    {
      widgets_->debugDraw();
    }
#endif
  }


  void makeQueryWidgets(const UI::WidgetPtr& widget) noexcept
  {
    if (widget->hasIdentifier())
    {
      query_widgets_.emplace(widget->getIdentifier(), widget);
      enumerated_widgets_.push_back(widget);
    }

    if (!widget->hasChild()) return;

    const auto& children = widget->getChildren();
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
        signalEventMessage(w, ":touch_began");
        // std::string event = w->getEvent() + ":touch_began";
        // Arguments args{
        //   { "widget", w->getIdentifier() }
        // };
        // event_.signal(event, args);

        first_touched_widget_ = w;
        touching_widget_ = w;
        touching_in_  = true;
        touch.handled = true;
        break;
      }
    }
  }
  
  void touchMoved(const Connection&, Arguments& arg) noexcept
  {
    if (!active_) return;
    if (first_touched_widget_.expired()) return;

    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);
    touch.handled = true;

    auto first_touch = first_touched_widget_.lock();
    auto widget      = touching_widget_.lock();

    touching_in_ = false;
    for (const auto& w : enumerated_widgets_)
    {
      if (!w->hasEvent()) continue;
      if (w != first_touch && !w->reactMoveEvent()) continue;

      bool contains = w->contains(pos);
      if (contains)
      {
        touching_in_ = true;
        if (w != widget)
        {
          // DOUT << "widget touch moved out-in: " << w->getIdentifier() << " " << ci::app::getElapsedFrames() << std::endl;
          signalEventMessage(w, ":moved_in");
          touching_widget_ = w;

          if (widget)
          {
            // Touch中のは強制キャンセル
            signalEventMessage(widget, ":moved_out");
          }
          break;
        }
      }
      else
      {
        if (w == widget)
        {
          // DOUT << "widget touch moved in-out: " << w->getIdentifier() << " " << ci::app::getElapsedFrames() << std::endl;
          signalEventMessage(w, ":moved_out");
          touching_widget_.reset();
        }
      }
    }
  }

  void touchEnded(const Connection&, Arguments& arg) noexcept
  {
    if (!active_) return;

    if (first_touched_widget_.expired()) return;
    first_touched_widget_.reset();

    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);
    touch.handled = true;

    if (touching_widget_.expired()) return;
    auto widget = touching_widget_.lock();

    if (widget->hasEvent() && widget->contains(pos))
    {
      // DOUT << "widget touch ended in: " << widget->getIdentifier() << std::endl;
      // イベント送信
      signalEventMessage(widget, ":touch_ended");

      if (widget->hasSe())
      {
        Arguments args{
          { "name", widget->getSe() }
        };
        event_.signal("UI:sound", args);
      }
    }

    touching_widget_.reset();
  }


  void multiTouchBegan(const Connection&, const Arguments& arg) noexcept
  {
    if (touching_widget_.expired()) return;

    // タッチ操作をキャンセル
    if (touching_in_)
    {
      // DOUT << "widget touch cancel: " << widget->getIdentifier() << std::endl;
      auto widget = touching_widget_.lock();
      signalEventMessage(widget, ":moved_out");
      touching_in_ = false;
    }
    first_touched_widget_.reset();
    touching_widget_.reset();
  }


  void signalEventMessage(const std::shared_ptr<Widget>& widget, const std::string& message)
  {
    Arguments args{
      { "widget", widget->getIdentifier() }
    };
    std::string event = widget->getEvent() + message;
    event_.signal(event, args);
  }
  

  glm::vec2 calcUIPosition(const glm::vec2& pos) noexcept
  {
    const auto& camera = camera_.body();
    auto ray = camera.generateRay(pos, ci::app::getWindowSize());

    float t;
    ray.calcPlaneIntersection(glm::vec3(), -unitZ(), &t);
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
  
  std::weak_ptr<UI::Widget> first_touched_widget_;
  std::weak_ptr<UI::Widget> touching_widget_;
  bool touching_in_ = false;

  UI::Drawer& drawer_;

  TweenCommon& tween_common_;

  bool active_ = true;

  ci::TimelineRef timeline_;
  TweenContainer tweens_;

#if defined (DEBUG)
  bool debug_info_ = false;
  bool debug_draw_ = false;
#endif

};


} }

