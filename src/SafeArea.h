#pragma once

//
// セーフエリアが存在するか調べる
// FIXME Canvas生成の度に調査している
//

namespace ngs { namespace SafeArea {

#if defined(CINDER_COCOA_TOUCH)

bool check();

#else

template <typename T = void>
bool check() { return false; }

#endif

} }
