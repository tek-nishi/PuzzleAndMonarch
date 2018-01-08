#pragma once

//
// アセット読み込み
//  OSX版のみDEBUGビルドで特殊なパスから読み込むようにしている
//

#include "Path.hpp"


namespace ngs { namespace Asset {

ci::DataSourceRef load(const std::string& path) noexcept {
  return ci::loadFile(getAssetPath(path));
}

} }
