#pragma once

//
// アプリ内パラメーター
//

#include <cinder/Json.h>
#include "Asset.hpp"
#include "TextCodec.hpp"


namespace ngs { namespace Params {

ci::JsonTree load(const std::string& path)
{
  return ci::JsonTree(Asset::load(path));
}

ci::JsonTree loadParams()
{
#if defined (OBFUSCATION_PARAMS)
  // 難読化されたファイル→JSON
  auto text = TextCodec::load(ci::app::getAssetPath("params.data").string());
  return ci::JsonTree(text);
#else
  // Jsonファイルをそのまま読む
  return load("params.json");
#endif
}

} }
