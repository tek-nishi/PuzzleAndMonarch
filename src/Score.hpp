#pragma once

//
// プレイ結果
//

namespace ngs {

struct Score
{
  // 各種スコア
  std::vector<u_int> scores;

  // 森と道は特別
  std::vector<u_int> forest;
  std::vector<u_int> path;

  // 集計スコア
  u_int total_score;
  u_int total_ranking;
  u_int total_panels;

  // 全てのパネルを置けた
  bool perfect;

  // パネル回転数と移動数
  u_int panel_turned_times;
  u_int panel_moved_times;

  // 制限時間
  double limit_time;
};

}
