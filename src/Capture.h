#pragma once

//
// UIViewのキャプチャ(iOS専用)
//

namespace ngs { namespace Capture {

#if defined(CINDER_COCOA_TOUCH)

bool canExec() noexcept;

UIImage* execute() noexcept;

#else

template <typename T = void>
bool canExec() noexcept { return false; }

template <typename T = void>
T* execute() noexcept { return nullptr; }

#endif

} }
