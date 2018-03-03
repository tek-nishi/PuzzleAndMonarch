#pragma once

//
// プレイ結果
//

namespace ngs {

struct Score
{
  std::vector<u_int> scores;

  u_int total_score;
  u_int total_ranking;
  u_int total_panels;

  u_int panel_turned_times;
  u_int panel_moved_times;

  // 制限時間
  double limit_time;

  bool high_score;
};

}
