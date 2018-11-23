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
constexpr T toRadians(const T& v) noexcept
{
  return v * float(M_PI) / 180.0f;
}

template <typename T>
constexpr T toDegrees(const T& v) noexcept
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
}

// 左シフト
template<typename T>
T rotateLeft(T x, unsigned int n) noexcept
{
  // s = n % (sizeof(T) * 8)
  unsigned int s = (n & ((sizeof(T) << 3) - 1));
  return (x << s) | (x >> ((sizeof(T) << 3) - s));
}


// コンテナへ追記
template<typename T1, typename T2>
void appendContainer(const T1& src, T2& dst) noexcept
{
  std::copy(std::begin(src), std::end(src), std::back_inserter(dst));
}

// キーワード置換
std::string replaceString(std::string text, const std::string& src, const std::string& dst) noexcept
{
  std::string::size_type pos = 0;
  while ((pos = text.find(src, pos)) != std::string::npos)
  {
    text.replace(pos, src.length(), dst);
    pos += dst.length();
  }

  return text;
}

// テキストで日付を取得
std::string getFormattedDate() noexcept
{
  auto t = std::time(nullptr);
  // TIPS ポインタから変数を生成
  auto tm = *std::localtime(&t);

  std::stringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");

  return { ss.str() };
}


// glm::vec3の定数
const glm::vec3& unitX() 
{
  static glm::vec3 v{ 1, 0, 0 };
  return v;
}

const glm::vec3& unitY() 
{
  static glm::vec3 v{ 0, 1, 0 };
  return v;
}

const glm::vec3& unitZ() 
{
  static glm::vec3 v{ 0, 0, 1 };
  return v;
}

// vec3.xz = vec2
template <typename T>
glm::vec3 vec2ToVec3(const T& v)
{
  return { v.x, 0, v.y };
}


float randFromVec2(const glm::vec2& v)
{
  return ci::randFloat(v.x, v.y);
}

ci::ColorA mulColor(const ci::ColorA& a, const ci::Color &b) noexcept
{
  return { a.r * b.r, a.g * b.g, a.b * b.b, a.a };
}

// 値が存在すればその値を、なければ初期値を返す
template <typename T>
T getValue(const Arguments& args, const std::string& key, const T& value)
{
  if (args.count(key))
  {
    return boost::any_cast<T>(args.at(key));
  }
  return value;
}

}
