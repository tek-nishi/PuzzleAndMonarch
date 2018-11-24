#pragma once

//
// Cocoa環境専用処理
//

#include "Defines.hpp"
#import <Foundation/Foundation.h>


namespace ngs {

NSString* createString(const std::string& text);

}
