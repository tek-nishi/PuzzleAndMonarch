#pragma once

//
// 動作確認用
//

#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Camera.hpp"
#include "ConnectionHolder.hpp"
#include "UICanvas.hpp"
#include "Params.hpp"


namespace ngs {

class TestPart
{


public:
  TestPart(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event),
      world_camera_(params["test.camera"]),
      distance_(params.getValueForKey<float>("test.camera.distance")),
      target_(Json::getVec<glm::vec3>(params["test.camera.target"])),
      drawer_(params["ui"]),
      canvas_(event, drawer_,
              params["ui.camera"],
              Params::load(params.getValueForKey<std::string>("ui_test.canvas.widgets")))
  {
    // World
    glm::vec3 eye = target_ + glm::vec3(0, 0, distance_);
    world_camera_.body().lookAt(eye, target_);

    // せっせとイベントを登録
    // holder_ += event_.connect("single_touch_began",
    //                           [this](const Connection&, const Arguments& arg) noexcept
    //                           {
    //                           });

    holder_ += event_.connect("single_touch_moved",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                auto& touch = boost::any_cast<Touch&>(arg.at("touch"));
                                if (touch.handled) return;

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
    
    // holder_ += event_.connect("single_touch_ended",
    //                           [this](const Connection&, const Arguments& arg) noexcept
    //                           {
    //                           });

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
    canvas_.resize();
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
      ci::gl::disableAlphaBlending();

      ci::gl::setMatrices(world_camera_.body());
      ci::gl::rotate(rot_);

      ci::gl::drawColorCube(glm::vec3(0), glm::vec3(1));
    }

    // UI
    {
      ci::gl::enableDepth(false);
      ci::gl::disable(GL_CULL_FACE);
      ci::gl::enableAlphaBlending();

      canvas_.draw();
    }
  }


private:


  // メンバ変数を最後尾で定義する実験
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  // World
  Camera world_camera_;

  float distance_;
  glm::vec3 target_;
  glm::quat rot_;
  

  // UI
  UI::Drawer drawer_;
  UI::Canvas canvas_;
};

}
