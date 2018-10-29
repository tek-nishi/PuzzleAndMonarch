#pragma once

//
// ゲームロジック
//

#include "Panel.hpp"
#include "Field.hpp"
#include <set>


namespace ngs {

// Fieldにパネルが置けるか判定
bool canPutPanel(const Panel& panel, const glm::ivec2& pos, u_int rotation, const Field& field) noexcept
{
  // 時計回りに調べる
  static const glm::ivec2 offsets[] = {
    {  0,  1 },
    {  1,  0 },
    {  0, -1 },
    { -1,  0 },
  };

  // ４隅の情報
  auto edge = panel.getRotatedEdgeValue(rotation);
  auto field_edge = edge;

  for (u_int i = 0; i < 4; ++i)
  {
    glm::ivec2 p = pos + offsets[i];
    if (!field.existsPanel(p)) continue;

    // Field上のパネル情報
    const auto& panel_status = field.getPanelStatus(p);
    auto field_panel_edge    = rotateLeft(panel_status.edge, 32);

    field_edge = (field_edge & ~(uint64_t(Panel::EDGE_MASK) << (i * 16))) | (field_panel_edge & (uint64_t(Panel::EDGE_MASK) << (i * 16)));  
  }

  return edge == field_edge;
}


// 調査済みエッジ
struct CheckedEdge
{
  glm::ivec2 pos;
  u_int direction;
};

bool isCheckedEdge(const glm::ivec2& pos, u_int direction, const std::vector<CheckedEdge>& checked)
{
  return std::any_of(std::begin(checked), std::end(checked),
                     [&pos, direction](const CheckedEdge& value)
                     {
                       return (value.pos == pos) && (value.direction == direction);
                     });
}


// パネル属性の端を調べる
bool checkAttributeEdge(const glm::ivec2& pos, u_int direction,
                        u_int attribute,
                        const Field& field, const std::vector<Panel>& panels,
                        std::vector<glm::ivec2>& checked,
                        std::vector<CheckedEdge>& checked_edge,
                        std::vector<glm::ivec2>& completed) noexcept
{
  // パネルがない→閉じていない
  if (!field.existsPanel(pos)) return false;

  // パネル情報
  const auto& status = field.getPanelStatus(pos);
  const auto& panel  = panels[status.number];
  const auto edge    = panel.getRotatedEdge(status.rotation);

  u_int dir = (direction + 2) % 4;

  // そこが端なら判定完了
  if (edge[dir] & Panel::EDGE)
  {
    // 調査済みEdge
    if (isCheckedEdge(pos, dir, checked_edge))
    {
      return false;
    }

    checked_edge.push_back({ pos, dir });
    completed.push_back(pos);
    return true;
  }

  // 調査ずみ？
  bool is_checked = std::any_of(std::begin(checked), std::end(checked),
                                [pos](const auto& it)
                                {
                                  return it == pos;
                                });
  if (is_checked)
  {
    // 調査続行
    return true;
  }
  checked.push_back(pos);

  // 時計回りに調べる
  static const glm::ivec2 offsets[] = {
    {  0,  1 },
    {  1,  0 },
    {  0, -1 },
    { -1,  0 },
  };

  // NOTICE 必ず１編は同じ属性がある
  for (u_int i = 0; i < 4; ++i)
  {
    // 戻らない
    if (i == dir) continue;
    if (!(edge[i] & attribute)) continue;
    
    // その先が閉じているか調査
    auto p = pos + offsets[i];
    if (!checkAttributeEdge(p, i, attribute, field, panels, checked, checked_edge, completed)) return false;
  }

  completed.push_back(pos);

  return true;
}

// 属性が完成したか調べる
std::vector<std::vector<glm::ivec2>> isCompleteAttribute(u_int attribute,
                                                         const glm::ivec2& pos,
                                                         const Field& field, const std::vector<Panel>& panels) noexcept
{
  std::vector<std::vector<glm::ivec2>> completed;

  // パネル情報
  const auto& status = field.getPanelStatus(pos);
  const auto& panel  = panels[status.number];
  const auto edge    = panel.getRotatedEdge(status.rotation);

  // パネルは端か途中かの２択(両方含んだ道は無い)
  bool has_attr = false;
  bool has_edge = false;
  for (u_int i = 0; i < 4; ++i)
  {
    if (edge[i] & attribute)
    {
      has_attr = true;
      if (edge[i] & Panel::EDGE)
      {
        has_edge = true;
        break;
      }
    }
  }

  // ４辺とも対象ではない
  if (!has_attr) return completed;

  // 時計回りに調べる
  static const glm::ivec2 offsets[] = {
    {  0,  1 },
    {  1,  0 },
    {  0, -1 },
    { -1,  0 },
  };

  if (has_edge)
  {
    // 端を含んでいる→端ごとに調査
    std::vector<CheckedEdge> checked_edge;

    for (u_int i = 0; i < 4; ++i)
    {
      if (!(edge[i] & attribute)) continue;
      if (isCheckedEdge(pos, i, checked_edge)) continue;

      checked_edge.push_back({ pos, i });

      // その先が閉じているか調査
      std::vector<glm::ivec2> checked;
      std::vector<glm::ivec2> comp;
      auto p = pos + offsets[i];
      if (checkAttributeEdge(p, i, attribute, field, panels, checked, checked_edge, comp))
      {
        comp.push_back(pos);
        completed.push_back(comp);
      }
    }
  }
  else
  {
    // 端を含んでいない→その先すべてで閉じていないとならない
    std::vector<glm::ivec2> checked;
    std::vector<CheckedEdge> checked_edge;

    checked.push_back(pos);

    std::vector<glm::ivec2> comp;
    for (u_int i = 0; i < 4; ++i)
    {
      auto p = pos + offsets[i];
      if (edge[i] & attribute)
      {
        // 開始位置は外す
        // auto it = std::find(std::begin(checked), std::end(checked), pos);
        // if (it != std::end(checked)) {
        //   checked.erase(it);
        // }

        // その先が閉じているか調査
        if (!checkAttributeEdge(p, i, attribute, field, panels, checked, checked_edge, comp))
        {
          // １つでも閉じていなければ調査完了
          return completed;
        }
      }
    }
    if (!comp.empty())
    {
      comp.push_back(pos);
      completed.push_back(comp);
    }
  }

  return completed;
}


// 周囲８箇所にパネルがあるか調査
bool isPanelAroundPos(const glm::ivec2& pos, const Field& field) noexcept
{
  // 周囲を時計回りに調べる
  static const glm::ivec2 offsets[] = {
    {  0,  1 },
    {  1,  1 },
    {  1,  0 },
    {  1, -1 },
    {  0, -1 },
    { -1, -1 },
    { -1,  0 },
    { -1,  1 },
  };

  return std::all_of(std::begin(offsets), std::end(offsets),
                     [&pos, &field](const auto ofs)
                     {
                       return field.existsPanel(pos + ofs);
                     });
}

// 教会が完成したか調査
std::vector<glm::ivec2> isCompleteChurch(const glm::ivec2& pos,
                                         const Field& field, const std::vector<Panel>& panels) noexcept
{
  std::vector<glm::ivec2> completed;

  static const glm::ivec2 offsets[] = {
    {  0,  0 },
    {  0,  1 },
    {  1,  1 },
    {  1,  0 },
    {  1, -1 },
    {  0, -1 },
    { -1, -1 },
    { -1,  0 },
    { -1,  1 },
  };

  for (const auto& ofs : offsets)
  {
    auto p = pos + ofs;
    if (!field.existsPanel(p)) continue;

    const auto& status = field.getPanelStatus(p);
    const auto& panel  = panels[status.number];
    if (panel.getAttribute() & Panel::CHURCH)
    {
      if (isPanelAroundPos(p, field))
      {
        completed.push_back(p);
      }
    }
  }
  return completed;
}

// 深い森を数える
u_int countDeepForest(const std::vector<glm::ivec2>& completed,
                      const Field& field, const std::vector<Panel>& panels) noexcept
{
  u_int count = 0;

  for (const auto& pos : completed)
  {
    const auto& status = field.getPanelStatus(pos);
    const auto& panel  = panels[status.number];
    if (panel.getAttribute() & Panel::DEEP_FOREST) ++count;
  }

  return count;
}


// 森(or道)総面積
int countTotalAttribute(const std::vector<std::vector<glm::ivec2>>& completed,
                        const Field& field, const std::vector<Panel>& panels) noexcept
{
  if (completed.empty()) return 0;

  int num = 0;
  for (const auto& comp : completed)
  {
    // TIPS 同じ場所にあるを森は再カウントしない
    std::set<glm::ivec2, LessVec<glm::ivec2>> area;
    for (const auto& p : comp)
    {
      area.insert(p);
    }
    num += area.size();
  }
  return num;
}

// 完成した街の数を数える
int countTown(const std::vector<std::vector<glm::ivec2>>& completed,
              const Field& field, const std::vector<Panel>& panels) noexcept
{
  // 道が１本も完成していない状態
  if (completed.empty()) return 0;

  // TIPS 同じ場所にある街を再カウントしない
  std::set<glm::ivec2, LessVec<glm::ivec2>> town_num;

  for (const auto& comp : completed)
  {
    for (const auto& p : comp)
    {
      const auto& status = field.getPanelStatus(p);
      if (panels[status.number].getAttribute() & (Panel::TOWN | Panel::CASTLE))
      {
        town_num.insert(p);
      }
    }
  }
  return int(town_num.size());
}


// 手持ちのパネルがフィールドにおけるか調べる
bool canPanelPutField(const Panel& panel, const std::vector<glm::ivec2>& blank, const Field& field) noexcept
{
  // 総当たりで調査
  for (const auto& pos : blank)
  {
    for (u_int i = 0; i < 4; ++i)
    {
      if (canPutPanel(panel, pos, i, field))
      {
        return true;
      }
    }
  }
  return false;
}

}
