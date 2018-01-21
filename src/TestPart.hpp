#pragma once

//
// 動作確認用
//

#include "ConnectionHolder.hpp"
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace ngs {

class TestPart
{


public:
  TestPart(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event),
      fov(params.getValueForKey<float>("test.camera.fov")),
      near_z(params.getValueForKey<float>("test.camera.near_z")),
      far_z(params.getValueForKey<float>("test.camera.far_z")),
      distance_(params.getValueForKey<float>("test.camera.distance")),
      target_(Json::getVec<glm::vec3>(params["test.camera.target"])),
      camera(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov, near_z, far_z)
  {
    glm::vec3 eye = target_ + glm::vec3(0, 0, distance_);
    camera.lookAt(eye, target_);

    // せっせとイベントを登録
    holder_ += event_.connect("single_touch_began",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                              });

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

                                float l      = glm::distance(touches[0].pos, touches[1].pos);
                                float prev_l = glm::distance(touches[0].prev_pos, touches[1].prev_pos);
                                float dl = l - prev_l;
                                if (std::abs(dl) > 1.0f)
                                {
                                  distance_ = std::max(distance_ - dl * 0.25f, camera.getNearClip() + 1.0f);
                                  glm::vec3 eye = target_ + glm::vec3(0, 0, distance_);
                                  camera.lookAt(eye, target_);
                                }
                              });
    
    holder_ += event_.connect("single_touch_ended",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                              });

  }


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


  void resize(float aspect) noexcept
  {
    camera.setAspectRatio(aspect);
    if (aspect < 1.0) {
      float half_w = std::tan(ci::toRadians(fov / 2)) * near_z;
      float half_h = half_w / aspect;
      float fov_w = std::atan(half_h / near_z) * 2;
      camera.setFov(ci::toDegrees(fov_w));
    }
    else {
      camera.setFov(fov);
    }
  }


  void update() noexcept
  {
  }

  void draw(glm::ivec2 window_size) noexcept
  {
    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);
    ci::gl::enableAlphaBlending();
    ci::gl::setMatrices(camera);

    ci::gl::rotate(rot_);

    ci::gl::drawColorCube(glm::vec3(0), glm::vec3(1));
  }


private:
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  float fov;
  float near_z;
  float far_z;

  float distance_;
  glm::vec3 target_;

  ci::CameraPersp camera;
  glm::quat rot_;
  

};

}
