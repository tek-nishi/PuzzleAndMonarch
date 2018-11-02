//
// OSごとの差異を吸収
//

#include "Defines.hpp"
#include <Foundation/Foundation.h>
#if !defined (CINDER_COCOA_TOUCH)
#include <AppKit/AppKit.h>
#endif
#include "Os.hpp" 


namespace ngs { namespace Os {


static NSString* createString(const std::string& text)
{
  NSString* str = [[[NSString alloc] initWithCString:text.c_str() encoding:NSUTF8StringEncoding] autorelease];
  return str;
}


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
