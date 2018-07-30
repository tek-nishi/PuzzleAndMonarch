//
// ローカライズ済み文字列を取り出す
//

#include "Defines.hpp"
#include <Foundation/Foundation.h>
#include "Localize.h" 


namespace ngs { namespace Localize {

// 投稿用の文字列(ローカライズ済み)を取得
std::string get(const std::string& key)
{
  NSString* key_text = [NSString stringWithUTF8String:key.c_str()];
  
  NSString* text = NSLocalizedString(key_text, nil);
  return [text UTF8String];
}

} }
