#pragma once

//
// ランキングを数値から表示文字列へ変換する
//   結果画面とランキング画面で使う
//

namespace ngs {

void convertRankToText(u_int rank, UI::Canvas& canvas, const std::string& id, const std::vector<std::string>& ranking_text)
{
  canvas.setWidgetText(id, ranking_text[rank]);
}

}
