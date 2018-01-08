#pragma once

//
// 地形パネル
//

#include <vector>


namespace ngs {

struct Panel {
  enum {
    // ４辺の構造
    PATH   = 1 << 0,         // 道
    GRASS  = 1 << 1,         // 草原
    FOREST = 1 << 2,         // 森

    EDGE = 1 << 16,          // 端

    // パネルの属性
    TOWN        = 1 << 0,         // 道の途中にある街
    CHURCH      = 1 << 1,         // 教会
    CASTLE      = 1 << 2,         // お城(開始位置)
    DEEP_FOREST = 1 << 3,         // 深い森
  };

  Panel(u_int attribute, u_int edge_up, u_int edge_right, u_int edge_bottom, u_int edge_left)
    : attribute_(attribute),
      edge_(4)
  {
    edge_[0] = edge_bottom;
    edge_[1] = edge_right;
    edge_[2] = edge_up;
    edge_[3] = edge_left;
  }


  u_int getAttribute() const { return attribute_; }

  const std::vector<u_int>& getEdge() const { return edge_; }

  // 回転ずみの端情報
  std::vector<u_int> getRotatedEdge(u_int rotation) const {
    std::vector<u_int> edge = edge_;

    // 左方向へのシフト
    std::rotate(std::begin(edge), std::begin(edge) + rotation, std::end(edge));
    return edge;
  }


private:
  u_int attribute_;
  std::vector<u_int> edge_;     // ４辺の構造

};



// 初期パネル生成
std::vector<Panel> createPanels() {
  std::vector<Panel> panels = {
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
    { 0, Panel::PATH,   Panel::FOREST | Panel::EDGE, Panel::PATH,  Panel::GRASS },
    { 0, Panel::FOREST, Panel::GRASS,  Panel::GRASS, Panel::FOREST },
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },
   
    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::PATH | Panel::EDGE,  Panel::PATH | Panel::EDGE,   Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { Panel::CASTLE, Panel::GRASS, Panel::GRASS,  Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::FOREST | Panel::EDGE, Panel::GRASS },

    // d
    { 0, Panel::GRASS,  Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::GRASS,  Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST, Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS,  Panel::FOREST, Panel::GRASS,  Panel::FOREST },
    
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH,   Panel::FOREST | Panel::EDGE, Panel::PATH,  Panel::GRASS },
    { 0, Panel::FOREST, Panel::GRASS,  Panel::GRASS, Panel::FOREST },
    { Panel::CHURCH, Panel::GRASS,  Panel::GRASS,  Panel::GRASS, Panel::GRASS },

    { 0, Panel::GRASS, Panel::PATH,   Panel::GRASS,  Panel::PATH },
    { Panel::TOWN, Panel::PATH | Panel::EDGE,  Panel::PATH | Panel::EDGE,   Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { Panel::CASTLE, Panel::GRASS, Panel::GRASS,  Panel::GRASS,  Panel::PATH | Panel::EDGE },
    { 0, Panel::GRASS, Panel::FOREST | Panel::EDGE, Panel::FOREST | Panel::EDGE, Panel::GRASS },

    // f 
    { 0, Panel::PATH,   Panel::FOREST, Panel::FOREST, Panel::PATH },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::GRASS,  Panel::FOREST, Panel::FOREST },
    { 0, Panel::GRASS,  Panel::GRASS,  Panel::FOREST | Panel::EDGE, Panel::GRASS },
    { 0, Panel::PATH,   Panel::FOREST | Panel::EDGE, Panel::PATH,   Panel::GRASS },
    
    { 0, Panel::FOREST, Panel::FOREST, Panel::PATH,   Panel::PATH },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST, Panel::FOREST },
    { Panel::DEEP_FOREST, Panel::FOREST, Panel::PATH | Panel::EDGE,   Panel::FOREST, Panel::FOREST },
    { Panel::TOWN, Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE,   Panel::PATH | Panel::EDGE },

    { 0, Panel::PATH, Panel::GRASS, Panel::PATH, Panel::GRASS },
    { 0, Panel::FOREST, Panel::GRASS, Panel::FOREST, Panel::GRASS },
    { 0, Panel::FOREST | Panel::EDGE, Panel::GRASS, Panel::GRASS, Panel::GRASS },
    { 0, Panel::PATH, Panel::FOREST | Panel::EDGE, Panel::PATH, Panel::GRASS },
  };

  return panels;
}

}

