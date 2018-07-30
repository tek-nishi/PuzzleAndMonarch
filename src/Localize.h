#pragma once

//
// ローカライズ済み文字列を取り出す
//

#include <string>


namespace ngs { namespace Localize {


#if defined (CINDER_COCOA_TOUCH) || defined (CINDER_MAC)

std::string get(const std::string& key);

#else

template<typename T>
T get(const T& key)
{
  return key;
}

#endif

} }
