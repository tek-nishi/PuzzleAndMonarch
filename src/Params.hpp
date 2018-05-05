#pragma once

//
// アプリ内パラメーター
//

#include <cinder/Json.h>
#include "Asset.hpp"
#include "PackedFile.hpp"


namespace ngs { namespace Params {

ci::JsonTree load(const std::string& path) noexcept
{
#if defined (USE_PACKED_FILE)
  // Packedから読む
  auto text = PackedFile::readString(path);
  return ci::JsonTree(text);
#else
  return ci::JsonTree(Asset::load(path));
#endif
}

} }
