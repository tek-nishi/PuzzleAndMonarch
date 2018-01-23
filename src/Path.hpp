#pragma once

//
// OSごとのパスの違いを吸収
//

#include "Defines.hpp"
#include <string>
#include <cinder/Utilities.h>
#include <cinder/app/App.h>


namespace ngs {

ci::fs::path getAssetPath(const std::string& path);
ci::fs::path getDocumentPath();


#if defined (NGS_PATH_IMPLEMENTATION)

ci::fs::path getAssetPath(const std::string& path) {
#if defined (DEBUG) && defined (CINDER_MAC)
  // Debug時、OSXはプロジェクトの場所からfileを読み込む
  ci::fs::path full_path(std::string(PREPRO_TO_STR(SRCROOT)) + "../assets/" + path);
  return full_path;
#else
  // TIPS:何気にci::app::getAssetPathがいくつかのpathを探してくれている
  auto full_path = ci::app::getAssetPath(path);
  return full_path;
#endif
}

ci::fs::path getDocumentPath() {
#if defined(CINDER_COCOA_TOUCH)
  // iOS版はアプリごとに用意された場所
  return ci::getDocumentsDirectory();
#elif defined (CINDER_MAC)
#if defined (DEBUG)
  // Debug時はプロジェクトの場所へ書き出す
  return ci::fs::path(PREPRO_TO_STR(SRCROOT));
#else
  // Release時はアプリコンテンツ内
  return ci::app::Platform::get()->getResourceDirectory();
#endif
#else
  // Widnowsは実行ファイルと同じ場所
  return ci::app::getAppPath();
#endif
}

#endif

}
