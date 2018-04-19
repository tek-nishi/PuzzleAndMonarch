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
      initial_rotation_(rotation_),
      initial_distance_(distance_),
      initial_position_(target_position_),
      field_center_(target_position_),
      field_distance_(distance_),
      map_center_(field_center_)
  {
  }

  ~FieldCamera() = default;


  void update(double delta_time)
  {
  }


  // 全リセット
  void resetAll() noexcept
  {
    force_camera_ = false;

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


  void addYaw(float yaw) noexcept
  {
    rotation_.y += yaw;
  }


  // フィールドの広さから注視点と距離を計算
  void calcViewRange(const glm::vec3& center, float radius, const Camera& camera)
  {
    map_center_ = center;

    {
      // ある程度の範囲が変更対象
      auto d = glm::distance(map_center_, target_position_);
      // 見た目の距離に変換
      auto dd = d / distance_;
      DOUT << "target_rate: " << dd << std::endl;
      if ((dd > target_rate_.x) && (dd < target_rate_.y))
      {
        field_center_.x = map_center_.x;
        field_center_.z = map_center_.z;
      }
    }
    
    float fov = camera.getFov();
    float distance = center / std::tan(ci::toRadians(fov * 0.5f));

    {
      // カメラが斜め上から見下ろしているのを考慮
      float n = d / std::cos(camera_rotation_.x);
      distance -= n;
    }

    {
      // 一定値以上遠のく場合は「ユーザー操作で意図的に離れている」
      // と判断する
      auto rate = distance / distance_;
      DOUT << "distace_rate: " << rate << std::endl;
      // if ((rate > distance_rate_.x) && (rate < distance_rate_.y))
      {
        field_distance_ = ci::clamp(distance,
                                    distance_range_.x, distance_range_.y);
      }
    }

    // 強制モード
    if (force_camera_)
    {
      field_center_.x = map_center_.x;
      field_center_.z = map_center_.z;
      field_distance_ = ci::clamp(distance, 
                                  distance_range_.x, distance_range_.y);
    }
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

  // カメラを初期状態に戻すための変数
  glm::vec2 initial_rotation_;
  float initial_distance_;
  glm::vec3 initial_target_position_;

  // Fieldの中心座標
  glm::vec3 field_center_;
  float field_distance_;

  glm::vec3 map_center_;

};

}
