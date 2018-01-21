#pragma once

//
// UI Test
//

#include <cinder/Camera.h>
#include <cinder/gl/GlslProg.h>
#include "UIWidgetsFactory.hpp"


namespace ngs {

class UITest {



public:
  UITest(const ci::JsonTree& params) noexcept
    : fov(params.getValueForKey<float>("ui_camera.fov")),
      near_z(params.getValueForKey<float>("ui_camera.near_z")),
      far_z(params.getValueForKey<float>("ui_camera.far_z")),
      camera(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov, near_z, far_z),
      widgets_(widgets_factory_.construct(params["ui_test.widgets"])),
      drawer_(params)
  {
    camera.lookAt(Json::getVec<glm::vec3>(params["ui_camera.eye"]),
                  Json::getVec<glm::vec3>(params["ui_camera.target"]));
  }


  void resize(float aspect) noexcept
  {
    camera.setAspectRatio(aspect);
    if (aspect < 1.0) {
      // 画面が縦長になったら、幅基準でfovを求める
      // fovとnear_zから投影面の幅の半分を求める
      float half_w = std::tan(ci::toRadians(fov / 2)) * near_z;

      // 表示画面の縦横比から、投影面の高さの半分を求める
      float half_h = half_w / aspect;

      // 投影面の高さの半分とnear_zから、fovが求まる
      float fov_w = std::atan(half_h / near_z) * 2;
      camera.setFov(ci::toDegrees(fov_w));
    }
    else {
      // 横長の場合、fovは固定
      camera.setFov(fov);
    }
  }


  void update() noexcept
  {
  }

  void draw(glm::ivec2 window_size) noexcept
  {
    ci::gl::enableDepth(false);
    ci::gl::disable(GL_CULL_FACE);
    ci::gl::enableAlphaBlending();
    ci::gl::setMatrices(camera);
    // ci::gl::ScopedGlslProg prog(shader);

    glm::vec3 top_left;
    glm::vec3 top_right;
    glm::vec3 bottom_left;
    glm::vec3 bottom_right;
    camera.getNearClipCoordinates(&top_left, &top_right, &bottom_left, &bottom_right);
    ci::Rectf rect(top_left.x, top_left.y, bottom_right.x, bottom_right.y);
    widgets_->draw(rect, glm::vec2(1, 1), drawer_);
  }


private:
  float fov;
  float near_z;
  float far_z;

  ci::CameraPersp camera;

  UI::WidgetPtr widgets_; 
  UI::WidgetsFactory widgets_factory_;

  UI::Drawer drawer_;
};

}
