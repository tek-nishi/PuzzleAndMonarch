//
// OSごとの差異を吸収
//

#include "Defines.hpp"
#include <Foundation/Foundation.h>
#include "Os.hpp" 


namespace ngs { namespace Os {

// 言語ファイルの取得
std::string lang()
{
  // NSString* key_text = [NSString stringWithUTF8String:key.c_str()];
  
  NSString* text = NSLocalizedString(@"LangFile", nil);
  return [text UTF8String];
}

} }
