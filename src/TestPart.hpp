#pragma once

//
// 動作確認用
//

#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Camera.hpp"
#include "ConnectionHolder.hpp"
#include "UIWidgetsFactory.hpp"


namespace ngs {

class TestPart
{


public:
  TestPart(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event),
      world_camera_(params["test.camera"]),
      distance_(params.getValueForKey<float>("test.camera.distance")),
      target_(Json::getVec<glm::vec3>(params["test.camera.target"])),
      ui_camera_(params["ui.camera"]),
      widgets_(widgets_factory_.construct(params["ui_test.widgets"])),
      drawer_(params["ui"])
  {
    glm::vec3 eye = target_ + glm::vec3(0, 0, distance_);
    world_camera_.body().lookAt(eye, target_);

    // せっせとイベントを登録
    // holder_ += event_.connect("single_touch_began",
    //                           [this](const Connection&, const Arguments& arg) noexcept
    //                           {
    //                           });

    holder_ += event_.connect("single_touch_moved",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                // タッチ位置の移動から回転軸と量を決める
                                auto d = touch.pos - touch.prev_pos;
                                float l = glm::length(d);
                                if (l > 0.0f)
                                {
                                  glm::vec3 axis(d.y, d.x, 0.0f);
                                  glm::quat q = glm::angleAxis(l * 0.01f, axis / l);
                                  rot_ = q * rot_;
                                }
                              });

    holder_ += event_.connect("multi_touch_moved",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& touches = boost::any_cast<const std::vector<Touch>&>(arg.at("touches"));
                                auto& camera = world_camera_.body();

                                float l      = glm::distance(touches[0].pos, touches[1].pos);
                                float prev_l = glm::distance(touches[0].prev_pos, touches[1].prev_pos);
                                float dl = l - prev_l;
                                if (std::abs(dl) > 1.0f)
                                {
                                  distance_ = std::max(distance_ - dl * 0.25f, camera.getNearClip() + 1.0f);
                                  glm::vec3 eye = target_ + glm::vec3(0, 0, distance_);
                                  camera.lookAt(eye, target_);
                                }
                                else
                                {
                                  auto v = touches[0].pos - touches[0].prev_pos;
                                  target_ += glm::vec3(-v.x, v.y, 0.0f) * 0.05f;

                                  glm::vec3 eye = target_ + glm::vec3(0, 0, distance_);
                                  camera.lookAt(eye, target_);
                                }
                              });
    
    holder_ += event_.connect("single_touch_ended",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                              });

    // UI
    ui_camera_.body().lookAt(Json::getVec<glm::vec3>(params["ui.camera.eye"]),
                             Json::getVec<glm::vec3>(params["ui.camera.target"]));

    // クエリ用情報生成
    makeQueryWidgets(widgets_);
    holder_ += event_.connect("single_touch_began",
                              std::bind(&TestPart::touchBegan,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("single_touch_moved",
                              std::bind(&TestPart::touchMoved,
                                        this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event_.connect("single_touch_ended",
                              std::bind(&TestPart::touchEnded,
                                        this, std::placeholders::_1, std::placeholders::_2));
  }

  ~TestPart() = default;


  void mouseMove(ci::app::MouseEvent event)
  {
  }

	void mouseDown(ci::app::MouseEvent event)
  {
  }

	void mouseDrag(ci::app::MouseEvent event)
  {
  }

	void mouseUp(ci::app::MouseEvent event)
  {
  }

  void mouseWheel(ci::app::MouseEvent event)
  {
  }

  void keyDown(ci::app::KeyEvent event)
  {
  }

  void keyUp(ci::app::KeyEvent event)
  {
  }


  void resize() noexcept
  {
    world_camera_.resize();
    ui_camera_.resize();
  }


  void update() noexcept
  {
  }

  void draw(glm::ivec2 window_size) noexcept
  {
    // World
    {
      ci::gl::enableDepth();
      ci::gl::enable(GL_CULL_FACE);
      ci::gl::enableAlphaBlending();
      ci::gl::setMatrices(world_camera_.body());

      ci::gl::rotate(rot_);

      ci::gl::drawColorCube(glm::vec3(0), glm::vec3(1));
    }

    // UI
    {
      ci::gl::enableDepth(false);
      ci::gl::disable(GL_CULL_FACE);
      ci::gl::enableAlphaBlending();

      auto& camera = ui_camera_.body();
      ci::gl::setMatrices(camera);

      glm::vec3 top_left;
      glm::vec3 top_right;
      glm::vec3 bottom_left;
      glm::vec3 bottom_right;
      camera.getNearClipCoordinates(&top_left, &top_right, &bottom_left, &bottom_right);
      ci::Rectf rect(top_left.x, top_left.y, bottom_right.x, bottom_right.y);
      
      widgets_->draw(rect, glm::vec2(1, 1), drawer_);
    }
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
  void touchBegan(const Connection&, const Arguments& arg) noexcept
  {
    const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);

    // 列挙したWidgetをクリックしたか計算
    for (const auto& w : enumerated_widgets_)
    {
      if (!w->hasEvent()) continue;

      if (w->contains(pos))
      {
        DOUT << "widget touch began: " << w->getIdentifier() << std::endl;
        touching_widget_ = w;
        touching_in_ = true;
        break;
      }
    }
  }
  
  void touchMoved(const Connection&, const Arguments& arg) noexcept
  {
    if (touching_widget_.expired()) return;

    const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);

    auto widget = touching_widget_.lock();
    bool contains = widget->contains(pos);
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
    touching_in_ = contains;
  }


  void touchEnded(const Connection&, const Arguments& arg) noexcept
  {
    if (touching_widget_.expired()) return;

    const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
    auto pos = calcUIPosition(touch.pos);

    auto widget = touching_widget_.lock();
    if (widget->contains(pos))
    {
      DOUT << "widget touch ended in: " << widget->getIdentifier() << std::endl;
    }
    else
    {
      DOUT << "widget touch ended out: " << widget->getIdentifier() << std::endl;
    }
  }

  std::weak_ptr<UI::Widget> touching_widget_;
  bool touching_in_ = false;


  glm::vec2 calcUIPosition(const glm::vec2& pos) noexcept
  {
    const auto& camera = ui_camera_.body();
    auto ray = camera.generateRay(pos, ci::app::getWindowSize());

    float t;
    ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 0, -1), &t);
    auto touch_pos = ray.calcPosition(t);
    // FIXME 暗黙の変換(vec3→vec2)
    glm::vec2 ui_pos = touch_pos;

    return ui_pos;
  }



  // メンバ変数を最後尾で定義する実験
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  // World
  Camera world_camera_;

  float distance_;
  glm::vec3 target_;
  glm::quat rot_;
  

  // UI
  Camera ui_camera_;

  UI::WidgetPtr widgets_; 
  UI::WidgetsFactory widgets_factory_;

  // クエリ用
  std::map<std::string, UI::WidgetPtr> query_widgets_;
  std::vector<UI::WidgetPtr> enumerated_widgets_;

  UI::Drawer drawer_;
};

}
