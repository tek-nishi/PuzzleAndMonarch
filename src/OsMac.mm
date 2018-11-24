//
// OSごとの差異を吸収
//

#include "Defines.hpp"
#include <Foundation/Foundation.h>
#if !defined (CINDER_COCOA_TOUCH)
#include <AppKit/AppKit.h>
#endif
#include "Os.hpp" 
#include "Cocoa.h" 


namespace ngs { namespace Os {

// 言語ファイルの取得
std::string lang()
{
  // NSString* key_text = [NSString stringWithUTF8String:key.c_str()];
  
  NSString* text = NSLocalizedString(@"LangFile", nil);
  return [text UTF8String];
}

#if defined (CINDER_COCOA_TOUCH)

void openURL(const std::string& url)
{
  [[UIApplication sharedApplication] openURL:[NSURL URLWithString:createString(url) ]];
}

#else

void openURL(const std::string& url)
{
  [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:createString(url) ]];
}

#endif

} }
