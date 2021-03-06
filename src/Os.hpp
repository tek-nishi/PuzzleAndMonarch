﻿#pragma once

//
// Osごとの差異を吸収
//

#include <string>


namespace ngs { namespace Os {

// 言語ファイルの取得
std::string lang();

// URLを開く
void openURL(const std::string& url);

} }
