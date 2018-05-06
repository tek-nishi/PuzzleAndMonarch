#pragma once

//
// ランキングを数値から表示文字列へ変換する
//   結果画面とランキング画面で使う
//   NOTICE 画面からはみ出すかもしれないがご愛嬌
//

namespace ngs {

void convertRankToText(u_int rank, UI::Canvas& canvas, const std::string& id, const std::vector<std::string>& ranking_text)
{
  std::string rank_text;

  switch (rank)
  {
  case 0:
    // スコア無し→ランク:0
    break;

  case 1:
    // 最低ランク
    rank_text = ranking_text[0];
    break;

  default:
    {
      rank -= 1;
      // ４進数へ変換
      int row  = rank % 4;
      int high = rank / 4;

      DOUT << "Rank: " << rank << " " << high << ", " << row << std::endl;

      std::ostringstream str;
      for (int i = 0; i < high; ++i)
      {
        str << ranking_text[2];
      }
      for (int i = 0; i < row; ++i)
      {
        str << ranking_text[1];
      }
      rank_text = str.str();
    }
  }

  canvas.setWidgetText(id, rank_text);
}

}
