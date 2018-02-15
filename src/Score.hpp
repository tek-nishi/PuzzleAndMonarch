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

  bool high_score;
};

}
