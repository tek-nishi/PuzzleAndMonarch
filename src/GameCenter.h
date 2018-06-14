#pragma once

//
// GameCenter操作
//

#include "Defines.hpp"
#include <functional>


namespace ngs { namespace GameCenter {

#if defined (CINDER_COCOA_TOUCH)

void authenticateLocalPlayer(std::function<void()> start_callback,
                             std::function<void()> finish_callback);

bool isAuthenticated();

void showBoard(std::function<void()> start_callback,
               std::function<void()> finish_callback);

void submitScore(const int score, const int total_panel);

void submitAchievement(const std::string& identifier, const double complete_rate = 100.0,
                       const bool banner = true);

void writeCachedAchievement();
#if defined (DEBUG)
void resetAchievement();
#endif

#else

// TIPS:iOS以外の環境は、テンプレートを使ってダミー関数を用意している

template <typename T1, typename T2>
void authenticateLocalPlayer(T1 start_callback,
                             T2 finish_callback) {}

template <typename T = void>
bool isAuthenticated() { return false; }

template <typename T1, typename T2>
void showBoard(T1 start_callback,
               T2 finish_callback) {}

template <typename T>
void submitScore(const T, const T) {}

template <typename T1, typename T2 = double, typename T3 = bool>
void submitAchievement(T1 identifier, T2 complete_rate = 0, T3 banner = true) {}

template <typename T = void>
void writeCachedAchievement() {}

#if defined (DEBUG)
template <typename T = void>
void resetAchievement() {}
#endif

#endif

} }
