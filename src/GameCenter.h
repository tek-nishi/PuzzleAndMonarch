#pragma once

//
// GameCenter操作
//

#include "Defines.hpp"
#include <functional>


namespace ngs { namespace GameCenter {

#if defined(CINDER_COCOA_TOUCH)

void authenticateLocalPlayer(std::function<void()> start_callback,
                             std::function<void()> finish_callback) noexcept;

bool isAuthenticated() noexcept;

void showBoard(std::function<void()> start_callback,
               std::function<void()> finish_callback) noexcept;

void submitStageScore(const int stage,
                      const int score, const double clear_time) noexcept;

void submitScore(const int score, const int total_items) noexcept;

void submitAchievement(const std::string& identifier, const double complete_rate = 100.0,
                       const bool banner = true) noexcept;

void writeCachedAchievement() noexcept;


#ifdef DEBUG

void resetAchievement() noexcept;

#endif

#else

// TIPS:iOS以外の環境は、テンプレートを使ってダミー関数を用意している

template <typename T1, typename T2>
void authenticateLocalPlayer(T1 start_callback,
                             T2 finish_callback) noexcept {}

template <typename T = void>
bool isAuthenticated() noexcept { return false; }

template <typename T1, typename T2>
void showBoard(T1 start_callback,
               T2 finish_callback) noexcept {}

template <typename T1, typename T2, typename T3>
void submitStageScore(T1 stage,
                      T2 score, T3 clear_time) noexcept {}

template <typename T>
  void submitScore(const T score, const T total_items) noexcept {}

template <typename T1, typename T2 = double, typename T3 = bool>
void submitAchievement(T1 identifier, T2 complete_rate = 0, T3 banner = true) noexcept {}

template <typename T = void>
void writeCachedAchievement() noexcept {};


#ifdef DEBUG

template <typename T = void>
void resetAchievement() noexcept {}

#endif

#endif

} }
