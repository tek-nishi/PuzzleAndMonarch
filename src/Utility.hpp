#pragma once

//
// 雑多な処理
//


namespace ngs {

// xをyで丸めこむ
int roundValue(int x, int y) {
  return (x > 0) ? (x + y / 2) / y
                 : (x - y / 2) / y;
}


// 比較関数(a < b を計算する)
// SOURCE:http://tankuma.exblog.jp/11670448/
template <typename T>
struct LessVec {
  bool operator()(const T& lhs, const T& rhs) const {
    for (int i = 0; i < lhs.length(); ++i) {
      if (lhs[i] < rhs[i]) return true;
      if (lhs[i] > rhs[i]) return false;
    }

    return false;
  }
};


template<typename T>
T toRadians(T v) {
  return v * float(M_PI) / 180.0f;
}

}


void setupCamera(ci::CameraPersp& camera, const float aspect, const float fov)
{
  camera.setAspectRatio(aspect);
  float near_z = camera.getNearClip();

  if (aspect < 1.0)
  {
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

