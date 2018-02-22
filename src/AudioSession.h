#pragma once

//
// AudioSession設定
//

namespace ngs { namespace AudioSession { 

#if defined(CINDER_COCOA_TOUCH)

void begin() noexcept;
void end() noexcept;

#else

// iOS以外は実装無し
template <typename T = void>
void begin() noexcept {}

template <typename T = void>
void end() noexcept {}

#endif

} }
