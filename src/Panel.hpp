#pragma once

//
// 地形パネル
//

#include <vector>
#include "Utility.hpp"


namespace ngs {

struct Panel {
  enum {
    // ４辺の構造
    PATH   = 1 << 0,         // 道
    GRASS  = 1 << 1,         // 草原
    FOREST = 1 << 2,         // 森
    WATER  = 1 << 3,         // 水
    
    EDGE_MASK = (1 << 16) - 1, 

    EDGE = 1 << 16,          // 端

    // パネルの属性
    TOWN        = 1 << 0,         // 道の途中にある街
    CHURCH      = 1 << 1,         // 教会
    CASTLE      = 1 << 2,         // お城
    DEEP_FOREST = 1 << 3,         // 深い森
    START       = 1 << 4,         // 開始用
    FORT        = 1 << 5,         // 砦

    BUILDING    = TOWN | CASTLE | FORT    // PATH完成とみなす建築物
  };

  Panel(u_int attribute, u_int edge_up, u_int edge_right, u_int edge_bottom, u_int edge_left) noexcept
    : attribute_(attribute),
      edge_(4)
  {
    edge_[0] = edge_bottom;
    edge_[1] = edge_right;
    edge_[2] = edge_up;
    edge_[3] = edge_left;

    uint64_t edge = 0;
    for (u_int i = 0; i < edge_.size(); ++i)
    {
      edge |= uint64_t(edge_[i] & Panel::EDGE_MASK) << (16 * i);
    }
    edge_bundled_ = edge;
  }

  ~Panel() = default;


  u_int getAttribute() const noexcept
  {
    return attribute_;
  }

  const std::vector<u_int>& getEdge() const noexcept
  {
    return edge_;
  }

  uint64_t getEdgeBundled() const noexcept
  {
    return edge_bundled_;
  }

  // 回転ずみの端情報
  std::vector<u_int> getRotatedEdge(u_int rotation) const noexcept
  {
    std::vector<u_int> edge = edge_;

    // 左方向へのシフト
    std::rotate(std::begin(edge), std::begin(edge) + rotation, std::end(edge));
    return edge;
  }

  // uint64_t で返す
  uint64_t getRotatedEdgeValue(u_int rotation) const noexcept
  {
    return rotateRight(edge_bundled_, rotation * 16);
  }


private:
  u_int attribute_;
  std::vector<u_int> edge_;     // ４辺の構造
  uint64_t edge_bundled_;       // ４辺の構造(１つにまとめた値)

};


// 初期パネル生成
std::vector<Panel> createPanels() noexcept
{
  static const std::vector<Panel> panels = {
    // a
    { Panel::DEEP_FOREST, Panel::GRASS,  Panel::FOREST, Panel::FOREST, Panel::FOREST },
    { 0, Panel::PATH,   Panel::PATH,   Panel::FOREST, Panel::FOREST },
    { Panel::TOWN, Panel::FOREST | Panel::EDGE, Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS,  Panel::GRASS,  Panel::PATH,   Panel::PATH },
    
    { 0, Panel::FOREST | Panel::EDGE, Panel::GRASS,  Panel::GRASS, Panel::GRASS },
    { 0, Panel::FOREST, Panel::FOREST, Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH,   Panel::PATH,   Panel::GRASS, Panel::FOREST | Panel::EDGE },
    { 0, Panel::PATH,   Panel::GRASS,  Panel::GRASS, Panel::PATH },
    
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::GRASS, Panel::FOREST | Panel::EDGE },
    { 0, Panel::GRASS, Panel::PATH,   Panel::PATH,  Panel::FOREST | Panel::EDGE },
    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS, Panel::PATH },
    { 0, Panel::GRASS, Panel::GRASS,  Panel::PATH,  Panel::PATH },

    // a
    { Panel::DEEP_FOREST, Panel::GRASS,  Panel::FOREST, Panel::FOREST, Panel::FOREST },
    { 0, Panel::PATH,   Panel::PATH,   Panel::FOREST, Panel::FOREST },
    { Panel::TOWN, Panel::FOREST | Panel::EDGE, Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS,  Panel::GRASS,  Panel::PATH,   Panel::PATH },

    { 0, Panel::FOREST | Panel::EDGE, Panel::GRASS,  Panel::GRASS, Panel::GRASS },
    { 0, Panel::FOREST, Panel::FOREST, Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH,   Panel::PATH,   Panel::GRASS, Panel::FOREST | Panel::EDGE },
    { 0, Panel::PATH,   Panel::GRASS,  Panel::GRASS, Panel::PATH },
    
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::GRASS, Panel::FOREST | Panel::EDGE },
    { 0, Panel::GRASS, Panel::PATH,   Panel::PATH,  Panel::FOREST | Panel::EDGE },
    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS, Panel::PATH },
    { 0, Panel::GRASS, Panel::GRASS,  Panel::PATH,  Panel::PATH },

    // a
    { Panel::DEEP_FOREST, Panel::GRASS,  Panel::FOREST, Panel::FOREST, Panel::FOREST },
    { 0, Panel::PATH,   Panel::PATH,   Panel::FOREST, Panel::FOREST },
    { Panel::TOWN, Panel::FOREST | Panel::EDGE, Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS,  Panel::GRASS,  Panel::PATH,   Panel::PATH },

    { 0, Panel::FOREST | Panel::EDGE, Panel::GRASS,  Panel::GRASS, Panel::GRASS },
    { 0, Panel::FOREST, Panel::FOREST, Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH,   Panel::PATH,   Panel::GRASS, Panel::FOREST | Panel::EDGE },
    { 0, Panel::PATH,   Panel::GRASS,  Panel::GRASS, Panel::PATH },
    
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::GRASS, Panel::FOREST | Panel::EDGE },
    { 0, Panel::GRASS, Panel::PATH,   Panel::PATH,  Panel::FOREST | Panel::EDGE },
    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS, Panel::PATH },
    { 0, Panel::GRASS, Panel::GRASS,  Panel::PATH,  Panel::PATH },

    // d
    { 0, Panel::GRASS,  Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::GRASS,  Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST, Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS,  Panel::FOREST, Panel::GRASS,  Panel::FOREST },
    
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH, Panel::FOREST | Panel::EDGE, Panel::PATH,  Panel::GRASS },
    { 0, Panel::FOREST, Panel::GRASS,  Panel::GRASS, Panel::FOREST },
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },
   
    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::PATH | Panel::EDGE,  Panel::PATH | Panel::EDGE,   Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { Panel::FOREST, Panel::GRASS, Panel::GRASS,  Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::FOREST | Panel::EDGE, Panel::GRASS },

    // d
    { 0, Panel::GRASS,  Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::GRASS,  Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST, Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS,  Panel::FOREST, Panel::GRASS,  Panel::FOREST },
    
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH, Panel::FOREST | Panel::EDGE, Panel::PATH,  Panel::GRASS },
    { 0, Panel::FOREST, Panel::GRASS,  Panel::GRASS, Panel::FOREST },
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },

    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::PATH | Panel::EDGE,  Panel::PATH | Panel::EDGE,   Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { Panel::FOREST, Panel::GRASS, Panel::GRASS,  Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::FOREST | Panel::EDGE, Panel::GRASS },

    // f 
    { 0, Panel::PATH,   Panel::FOREST, Panel::FOREST, Panel::PATH },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::GRASS,  Panel::FOREST, Panel::FOREST },
    { 0, Panel::GRASS,  Panel::GRASS,  Panel::FOREST | Panel::EDGE, Panel::GRASS },
    { 0, Panel::PATH, Panel::FOREST | Panel::EDGE, Panel::PATH,   Panel::GRASS },
    
    { 0, Panel::FOREST, Panel::FOREST, Panel::PATH,   Panel::PATH },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::PATH | Panel::EDGE,   Panel::FOREST, Panel::FOREST },
    { Panel::CASTLE, Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },

    { 0, Panel::PATH, Panel::GRASS, Panel::PATH, Panel::GRASS },
    { 0, Panel::FOREST, Panel::GRASS, Panel::FOREST, Panel::GRASS },
    { 0, Panel::FOREST | Panel::EDGE, Panel::GRASS, Panel::GRASS, Panel::GRASS },
    { Panel::START, Panel::PATH, Panel::FOREST | Panel::EDGE, Panel::PATH, Panel::GRASS },
  };

  return panels;
}

}

