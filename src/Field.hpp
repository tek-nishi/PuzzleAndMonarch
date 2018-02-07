#pragma once

//
// パネルを置く場所
//

#include <map>
#include <glm/glm.hpp>
#include <algorithm>
#include "Utility.hpp"


namespace ngs {

// Fieldに置かれたパネルの状態
struct PanelStatus {
  glm::ivec2 position;
  int number;              // パネル番号
  u_int rotation;          // 0~3 時計回りに回転している状態
};


struct Field {

  // 置ける場所を探す
  std::vector<glm::ivec2> searchBlank() noexcept
  {
    std::vector<glm::ivec2> blank; 

    const glm::ivec2 offsets[] = {
      { -1,  0 },
      {  1,  0 },
      {  0, -1 },
      {  0,  1 },
    };

    // FIXME std::mapの列挙はあまり好ましくない
    for (const auto& it : panel_status_) {
      for (const auto& ofs : offsets) {
        auto p = it.first + ofs;
        if (!panel_status_.count(p)) {
          // 重複を調べてから追加
          if (std::find(std::begin(blank), std::end(blank), p) == std::end(blank)) {
            blank.push_back(p);
          }
        }
      }
    }

    return blank;
  }


  bool existsPanel(glm::ivec2 pos) const noexcept
  {
    return panel_status_.count(pos);
  }

  const PanelStatus& getPanelStatus(glm::ivec2 pos) const noexcept
  {
    return panel_status_.at(pos);
  }


  // 追加
  void addPanel(int number, glm::ivec2 pos, u_int rotation) noexcept
  {
    PanelStatus status = {
      pos,
      number,
      rotation
    };

    panel_status_.emplace(pos, status);
  }

  // フィールド上のパネルを列挙
  std::vector<PanelStatus> enumeratePanels() noexcept
  {
    std::vector<PanelStatus> panels;

    // FIXME std::mapの列挙はあまり好ましくない
    for (const auto& it : panel_status_) {
      panels.push_back(it.second);
    }
    return panels;
  }


private:
  // TIPS 座標をマップのキーにしている
  std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> panel_status_;

};

}
