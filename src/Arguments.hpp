#pragma once

//
// 汎用的な引数
//

#include <map>
#include <string>
#include "boost/any.hpp"


namespace ngs {

using Arguments = std::map<std::string, boost::any>;

bool hasKey(const Arguments& args, const std::string& key) noexcept {
  return args.count(key);
}

}
