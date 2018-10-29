#pragma once

//
// パネルを置く場所
//

#include <map>
#include <boost/noncopyable.hpp>
#include <glm/glm.hpp>
#include <algorithm>
#include "Utility.hpp"


namespace ngs {

// Fieldに置かれたパネルの状態
struct PanelStatus
{
  glm::ivec2 position;
  int number;              // パネル番号
  u_int rotation;          // 0~3 時計回りに回転している状態
  // 置いた状態でのパネル端情報
  uint64_t edge;
};


struct Field
{
  Field()  = default;
  ~Field() = default;


  // Jsonから状況を復元
  Field(const ci::JsonTree& json) noexcept
  {
    std::for_each(std::begin(json), std::end(json),
                  [this](const auto& obj)
                  {
                    auto number   = obj.template getValueForKey<int>("number");
                    auto pos      = Json::getVec<glm::ivec2>(obj["pos"]);
                    auto rotation = obj.template getValueForKey<u_int>("rotation");
                    auto edge     = Json::getValue<uint64_t>(obj, "edge", 0);

                    addPanel(number, pos, rotation, edge);
                  });
  }


  // 置ける場所を探す
  std::vector<glm::ivec2> searchBlank() const noexcept
  {
    // TIPS 重複する場所の列挙を回避できる
    std::set<glm::ivec2, LessVec<glm::ivec2>> blank_candidate;

    static const glm::ivec2 offsets[] = {
      { -1,  0 },
      {  1,  0 },
      {  0, -1 },
      {  0,  1 },
    };

    for (const auto& pos : panel_pos_array_)
    {
      for (const auto& ofs : offsets)
      {
        auto p = pos + ofs;
        if (!panel_status_.count(p))
        {
          blank_candidate.insert(p);
        }
      }
    }

    std::vector<glm::ivec2> blank;
    appendContainer(blank_candidate, blank);

    return blank;
  }


  bool existsPanel(const glm::ivec2& pos) const noexcept
  {
    return panel_status_.count(pos);
  }

  const PanelStatus& getPanelStatus(const glm::ivec2& pos) const noexcept
  {
    return panel_status_.at(pos);
  }

  // 追加
  void addPanel(int number, const glm::ivec2& pos, u_int rotation, uint64_t edge) noexcept
  {
    PanelStatus status = {
      pos,
      number,
      rotation,
      edge
    };

    panel_status_.emplace(pos, status);
    panel_pos_array_.push_back(pos);
  }

  std::vector<PanelStatus> enumeratePanels() const noexcept
  {
    std::vector<PanelStatus> panels;

    for (const auto& pos : panel_pos_array_)
    {
      const auto& status = panel_status_.at(pos);
      panels.emplace_back(status);
    }
    return panels;
  }
  
  const std::vector<glm::ivec2>& getPanelPositions() const noexcept
  {
    return panel_pos_array_;
  }


  ci::JsonTree serialize() const noexcept
  {
    ci::JsonTree data = ci::JsonTree::makeObject("field");

    for (const auto& pos : panel_pos_array_)
    {
      const auto& status = panel_status_.at(pos);

      ci::JsonTree p;
      p.addChild(Json::createFromVec("pos", status.position))
       .addChild(ci::JsonTree("number",     status.number))
       .addChild(ci::JsonTree("rotation",   status.rotation))
       .addChild(ci::JsonTree("edge",       status.edge))
      ;

      data.pushBack(p);
    }

    return data;
  }


private:
  // TIPS 座標をマップのキーにしている
  std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> panel_status_;
  // 置いた順序
  std::vector<glm::ivec2> panel_pos_array_;
};

}
