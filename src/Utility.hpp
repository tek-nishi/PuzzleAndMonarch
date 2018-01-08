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
