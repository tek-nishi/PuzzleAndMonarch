#pragma once

//
// セーフエリアが存在するか調べる
//

namespace ngs { namespace SafeArea {

#if defined(CINDER_COCOA_TOUCH)

bool check();

#else

template <typename T = void>
bool check() { return false; }

#endif

} }
