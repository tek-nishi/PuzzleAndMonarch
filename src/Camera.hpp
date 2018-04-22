#pragma once

//
// ci::CameraPerspの薄いラップクラス
//

#include <cinder/Camera.h>
#include <boost/noncopyable.hpp>


namespace ngs {

struct Camera
  : private boost::noncopyable
{
  Camera(const ci::JsonTree& params) noexcept
    : fov_(params.getValueForKey<float>("fov")),
      near_z_(params.getValueForKey<float>("near_z")),
      far_z_(params.getValueForKey<float>("far_z")),
      camera_(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov_, near_z_, far_z_)
  {
    // 縦画面の時は画角が違う
    resize();
  }

  ~Camera() = default;


  void resize() noexcept
  {
    float aspect = ci::app::getWindowAspectRatio();
    camera_.setAspectRatio(aspect);
    if (aspect < 1.0f)
    {
      // TIPS cinder0.9.1には便利なAPIがあった!!
      camera_.setFovHorizontal(fov_);
    }
    else
    {
      camera_.setFov(fov_);
    }
  }

  
  float getFov() const noexcept
  {
    // NOTICE 横長画面と縦長画面ではFOVは違う
    auto aspect = camera_.getAspectRatio();
    return (aspect < 1.0f) ? camera_.getFovHorizontal()
                           : camera_.getFov();
  }

  void setFov(float fov) noexcept
  {
    fov_ = fov;
    resize();
  }

  float getNearClip() const noexcept
  {
    return near_z_;
  }

  void setNearClip(float near_z) noexcept
  {
    near_z_ = near_z;
    camera_.setNearClip(near_z_);
    resize();
  }
  
  float getFarClip() const noexcept
  {
    return far_z_;
  }

  void setFarClip(float far_z) noexcept
  {
    far_z_ = far_z;
    camera_.setFarClip(far_z_);
  }


  // FIXME これ２つ書かなきゃいけないんですかね
  const ci::CameraPersp& body() const noexcept
  {
    return camera_;
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
