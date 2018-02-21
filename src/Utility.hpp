#pragma once

//
// 雑多な処理
//


namespace ngs {

// xをyで丸めこむ
int roundValue(int x, int y) noexcept
{
  return (x > 0) ? (x + y / 2) / y
                 : (x - y / 2) / y;
}

glm::ivec2 roundValue(int x, int y, int v) noexcept
{
  return glm::ivec2{ roundValue(x, v), roundValue(y, v) };
}


// 比較関数(a < b を計算する)
// SOURCE:http://tankuma.exblog.jp/11670448/
template <typename T>
struct LessVec
{
  bool operator()(const T& lhs, const T& rhs) const noexcept
  {
    for (int i = 0; i < lhs.length(); ++i)
    {
      if (lhs[i] < rhs[i]) return true;
      if (lhs[i] > rhs[i]) return false;
    }

    return false;
  }
};

// 配列の要素数を取得
template <typename T>
std::size_t elemsof(const T& value) noexcept
{
  return std::end(value) - std::begin(value);
}

template <typename T>
T toRadians(const T& v) noexcept
{
  return v * float(M_PI) / 180.0f;
}

template <typename T>
T toDegrees(const T& v) noexcept
{
  return v * 180.0f / float(M_PI);
}

// ビットローテート
// SOURCE http://qune.jp/archive/001213/index.html
template<typename T>
T rotateRight(T x, unsigned int n) noexcept
{
  // s = n % (sizeof(T) * 8)
  unsigned int s = (n & ((sizeof(T) << 3) - 1));
  return (x >> n) | (x << ((sizeof(T) << 3) - s));
};

// 左シフト
template<typename T>
T rotateLeft(T x, unsigned int n) noexcept
{
  // s = n % (sizeof(T) * 8)
  unsigned int s = (n & ((sizeof(T) << 3) - 1));
  return (x << s) | (x >> ((sizeof(T) << 3) - s));
};

}
