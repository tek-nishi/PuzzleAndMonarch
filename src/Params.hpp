#pragma once

//
// アプリ内パラメーター
//

#include <cinder/Json.h>
#include "Asset.hpp"


namespace ngs { namespace Params {

ci::JsonTree load(const std::string& path) {
  return ci::JsonTree(Asset::load(path));
}

} }
