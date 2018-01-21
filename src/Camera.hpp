#pragma once

//
// ci::CameraPerspの薄いラップクラス
//

#include <cinder/Camera.h>


namespace ngs {

struct Camera
{
  Camera(const ci::JsonTree& params) noexcept
    : fov_(params.getValueForKey<float>("fov")),
      near_z_(params.getValueForKey<float>("near_z")),
      far_z_(params.getValueForKey<float>("far_z")),
      camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov_, near_z_, far_z_)
  {
  }

  ~Camera() = default;


  void resize() noexcept
  {
    float aspect = ci::app::getWindowAspectRatio();
    camera_.setAspectRatio(aspect);
    if (aspect < 1.0f) {
      float half_w = std::tan(ci::toRadians(fov_ / 2)) * near_z_;
      float half_h = half_w / aspect;
      float fov_w  = std::atan(half_h / near_z_) * 2;
      camera_.setFov(ci::toDegrees(fov_w));
    }
    else {
      camera_.setFov(fov_);
    }
  }

  
  float getFov() const noexcept
  {
    return fov_;
  }

  void setFov(const float fov) noexcept
  {
    fov_ = fov;
    resize();
  }

  float getNearClip() const noexcept
  {
    return near_z_;
  }

  void setNearClip(const float near_z) noexcept
  {
    near_z_ = near_z;
    camera_.setNearClip(near_z_);
    resize();
  }
  
  float getFarClip() const noexcept
  {
    return far_z_;
  }

  void setFarClip(const float far_z) noexcept
  {
    far_z_ = far_z;
    camera_.setFarClip(far_z_);
  }


  ci::CameraPersp& body() noexcept
  {
    return camera_;
  }


private:
  float fov_;
  float near_z_;
  float far_z_;

  ci::CameraPersp camera_;
};

}
