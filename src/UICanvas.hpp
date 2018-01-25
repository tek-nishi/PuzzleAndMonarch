#pragma once

//
// UIの最上位
//

#include <map>
#include <string>
#include "UIWidgetsFactory.hpp"
#include "UIDrawer.hpp"
#include "Camera.hpp"


namespace ngs { namespace UI {

class Canvas
{

public:
  Canvas(Event<Arguments>& event,
         UI::Drawer& drawer,
         const ci::JsonTree& camera_params,
         const ci::JsonTree& widgets_params) noexcept
    : event_(event),
      drawer_(drawer),
      camera_(camera_params),
      widgets_(widgets_factory_.construct(widgets_params))
  {
    camera_.body().lookAt(glm::vec3(0, 0, camera_.getNearClip()), glm::vec3(0));

    makeQueryWidgets(widgets_);

    holder_ += event_.connect("single_touch_began",
                              std::bind(&Canvas::touchBegan,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("single_touch_moved",
                              std::bind(&Canvas::touchMoved,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("single_touch_ended",
                              std::bind(&Canvas::touchEnded,
                                        this, std::placeholders::_1, std::placeholders::_2));
  }

  ~Canvas() = default;


  void resize() noexcept
  {
    camera_.resize();
  }

  void draw() noexcept
  {
    auto& camera = camera_.body();
    ci::gl::setMatrices(camera);
#if !defined (CINDER_COCOA_TOUCH)
    // TIPS z値でカリングせずにclampする
    ci::gl::enable(GL_DEPTH_CLAMP);
#endif

    glm::vec3 top_left;
    glm::vec3 top_right;
    glm::vec3 bottom_left;
    glm::vec3 bottom_right;
    camera.getNearClipCoordinates(&top_left, &top_right, &bottom_left, &bottom_right);
    ci::Rectf rect(top_left.x, bottom_right.y, bottom_right.x, top_left.y);

    widgets_->draw(rect, glm::vec2(1), drawer_);
    
#if !defined (CINDER_COCOA_TOUCH)
    ci::gl::disable(GL_DEPTH_CLAMP);
#endif
  }


  const UI::WidgetPtr at(const std::string& name) const noexcept
  {
    return query_widgets_.at(name);
  }




private:
  void makeQueryWidgets(const UI::WidgetPtr& widget) noexcept
  {
    query_widgets_.insert({ widget->getIdentifier(), widget });
    enumerated_widgets_.push_back(widget);

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
    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);

    // 列挙したWidgetをクリックしたか計算
    for (const auto& w : enumerated_widgets_)
    {
      if (!w->hasEvent()) continue;

      if (w->contains(pos))
      {
        // DOUT << "widget touch began: " << w->getIdentifier() << std::endl;
        // std::string event = w->getEvent() + ":touch_began";
        // event_.signal(event, Arguments());
        touching_widget_ = w;
        touching_in_ = true;
        touch.handled = true;
        break;
      }
    }
  }
  
  void touchMoved(const Connection&, Arguments& arg) noexcept
  {
    if (touching_widget_.expired()) return;

    auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);
    touch.handled = true;

    auto widget = touching_widget_.lock();
    bool contains = widget->contains(pos);
#if 0
    // TODO 詳細な実装は後回し
    if (contains)
    {
      if (touching_in_)
      {
        DOUT << "widget touch moved in-in: " << widget->getIdentifier() << std::endl;
      }
      else
      {
        DOUT << "widget touch moved out-in: " << widget->getIdentifier() << std::endl;
      }
    }
    else
    {
      if (touching_in_)
      {
        DOUT << "widget touch moved in-out: " << widget->getIdentifier() << std::endl;
      }
      else
      {
        DOUT << "widget touch moved out-out: " << widget->getIdentifier() << std::endl;
      }
    }
#endif
    touching_in_ = contains;
  }


  void touchEnded(const Connection&, Arguments& arg) noexcept
  {
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
      // DOUT << "event: " << event << std::endl;
      event_.signal(event, Arguments());
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
};

} }

