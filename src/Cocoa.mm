//
// Cocoa環境専用処理
//

#include "Cocoa.h"


namespace ngs {

// std::string → NSString
NSString* createString(const std::string& text)
{
  return [NSString stringWithUTF8String:text.c_str()];

  // return [NSString stringWithCString:text.c_str()
  //                  encoding:NSUTF8StringEncoding];
}

}
