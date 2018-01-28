#pragma once

//
// 汎用使い捨てカウンタ
//   値が０になったら自動的に削除される変数
//

#include <map>
#include <string>
#include <boost/noncopyable.hpp>


namespace ngs {

struct Counter
  : private boost::noncopyable
{
  Counter()  = default;
  ~Counter() = default;


  void add(const std::string& name, const double count) noexcept
  {
    values.insert({ name, count });
  }

  bool check(const std::string& name) const noexcept
  {
    return values.count(name);
  }

  double get(const std::string& name) const noexcept
  {
    if (!check(name)) return 0.0;
    return values.at(name);
  }


  void update(const double delta_time) noexcept
  {
    if (values.empty()) return;

    for (auto it = std::begin(values); it != std::end(values); )
    {
      it->second -= delta_time;

      if (it->second < 0.0)
      {
        // 値を削除
        it = values.erase(it);
        continue;
      }

      ++it;
    }
  }


private:
  std::map<std::string, double> values;

};

}
