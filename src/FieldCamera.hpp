#pragma once

//
// Field Camera
//

#include <boost/noncopyable.hpp>


namespace ngs {

class FieldCamera
  : private boost::noncopyable
{
public:
  FieldCamera(const ci::JsonTree& params)
    : rotation_(toRadians(Json::getVec<glm::vec2>(params["field.camera.rotation"]))),
      distance_(params.getValueForKey<float>("field.camera.distance")),
      target_position_(Json::getVec<glm::vec3>(params["field.target_position"])),
      distance_range_(Json::getVec<glm::vec2>(params["field.camera_distance_range"])),
      target_rate_(Json::getVec<glm::vec2>(params["field.target_rate"])),
      distance_rate_(Json::getVec<glm::vec2>(params["field.distance_rate"])),
      field_center_(target_position_),
      field_distance_(distance_),
      map_center_(field_center_),
      initial_rotation_(rotation_),
      initial_distance_(distance_),
      initial_target_position_(target_position_)
  {
  }

  ~FieldCamera() = default;


  void update(double delta_time)
  {
    target_position_ += (field_center_ - target_position_) * 0.025f;
    distance_ += (field_distance_ - distance_) * 0.05f;
  }


  // 全リセット
  void resetAll() noexcept
  {
    force_camera_ = false;

    reset();

    rotation_ = initial_rotation_;
    distance_ = initial_distance_;
    target_position_ = initial_target_position_;
  }

  // リセット(rotationは維持)
  void reset() noexcept
  {
    field_center_   = initial_target_position_;
    field_distance_ = initial_distance_;
  }


  void rotate(glm::vec3 pos, glm::vec3 prev_pos)
  {
    pos      = glm::normalize(pos - target_position_);
    prev_pos = glm::normalize(prev_pos - target_position_);

    // 外積から回転量が決まる
    float cross = prev_pos.x * pos.z - prev_pos.z * pos.x;
    rotation_.y += std::asin(cross);
  }

  void addYaw(float r)
  {
    rotation_.y += r;
  }


  // 距離設定
  void setDistance(float rate)
  {
    distance_ = ci::clamp(distance_ / rate,
                          distance_range_.x, distance_range_.y);
    field_distance_ = distance_;
  }

  // 平行移動
  void setTranslate(const glm::vec3& v, ci::CameraPersp& camera)
  {
    target_position_ += v;
    field_center_ = target_position_;
    eye_position_ += v;
    camera.setEyePoint(eye_position_);
  }

  void force(bool value)
  {
    force_camera_ = value;
  }


  // フィールドの広さから注視点と距離を計算
  void calcViewRange(const glm::vec3& center, float radius, const Camera& camera)
  {
    map_center_ = center;

    field_center_.x = map_center_.x;
    field_center_.z = map_center_.z;
    
    float fov = camera.getFov();
    float distance = radius / std::tan(ci::toRadians(fov * 0.5f));

    // カメラが斜め上から見下ろしているのを考慮
    float n = radius / std::cos(rotation_.x);
    distance -= n;

    field_distance_ = ci::clamp(distance,
                                distance_range_.x, distance_range_.y);

#if 0
    // 強制モード
    if (force_camera_)
    {
      field_center_.x = map_center_.x;
      field_center_.z = map_center_.z;

      DOUT << "field_distance: " << field_distance_ << std::endl;
    }
#endif
  }


  // 内容を他のクラスへ反映 
  void applyDetail(ci::CameraPersp& camera, View& view) 
  {
    glm::quat q(glm::vec3{ rotation_.x, rotation_.y, 0 });
    glm::vec3 p = q * glm::vec3{ 0, 0, -distance_ };
    camera.lookAt(p + target_position_, target_position_);
    eye_position_ = camera.getEyePoint();

    view.setupShadowCamera(map_center_);
  }


private:
  glm::vec2 rotation_;
  float distance_;

  // 注視位置
  glm::vec3 target_position_;
  // カメラ位置
  glm::vec3 eye_position_;

  // 距離を調整するための係数
  glm::vec2 target_rate_;
  glm::vec2 distance_rate_;
  // ピンチング操作時の距離の範囲
  glm::vec2 distance_range_;
  
  // カメラ計算を優先
  bool force_camera_ = false;

  // Fieldの中心座標
  glm::vec3 field_center_;
  float field_distance_;

  glm::vec3 map_center_;

  // カメラを初期状態に戻すための変数
  glm::vec2 initial_rotation_;
  float initial_distance_;
  glm::vec3 initial_target_position_;

};

}
