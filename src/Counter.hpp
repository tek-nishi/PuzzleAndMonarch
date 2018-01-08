#pragma once

//
// 汎用使い捨てカウンタ
//   値が０になったら自動的に削除される変数
//

#include <map>
#include <string>


namespace ngs {

struct Counter {

  void add(const std::string& name, u_int count) {
    values.insert({ name, count });
  }

  bool check(const std::string& name) const {
    return values.count(name);
  }

  u_int get(const std::string& name) const {
    if (!check(name)) return 0;
    return values.at(name);
  }


  void update() {
    if (values.empty()) return;

    for (auto it = std::begin(values); it != std::end(values); ) {
      if (!it->second) {
        // 値を削除
        it = values.erase(it);
        continue;
      }

      --it->second;
      ++it;
    }
  }


private:
  std::map<std::string, u_int> values;

};

}
