#pragma once

//
// 表示関連
//

#include <boost/noncopyable.hpp>
#include <deque>
#include <cinder/TriMesh.h>
#include <cinder/gl/Vbo.h>
#include "PLY.hpp"


namespace ngs {

enum {
  PANEL_SIZE = 20,
};


class View
  : private boost::noncopyable
{
  // パネル
  std::vector<std::string> panel_path;
  std::vector<ci::gl::VboMeshRef> panel_models;
  // AABBは全パネル共通
  ci::AxisAlignedBox panel_aabb_;

  // 演出用
  ci::gl::VboMeshRef blank_model;
  ci::gl::VboMeshRef selected_model;
  ci::gl::VboMeshRef cursor_model;

  // 背景
  ci::gl::VboMeshRef bg_model; 

  // Field上のパネル(Modelと被っている)
  struct Panel
  {
    glm::vec3 position;
    glm::vec3 rotation;
    int index;
  };
  // NOTICE 追加時にメモリ上で再配置されるのを避けるためstd::vectorではない
  std::deque<Panel> field_panels_;

  // 画面演出用情報
  float panel_height_;
  glm::vec2 put_duration_;
  std::string put_ease_;

  // 得点時演出用
  ci::gl::VboMeshRef effect_model;

  struct Effect {
    bool active;
    glm::vec3 pos;
    glm::vec3 rot;
  };
  // FIXME 途中の削除が多いのでvectorは向いていない??
  std::list<Effect> effects_;


  // 読まれてないパネルを読み込む
  const ci::gl::VboMeshRef& getPanelModel(int number) noexcept
  {
    if (!panel_models[number])
    {
      auto tri_mesh = PLY::load(panel_path[number]);
      // panel_aabb[number] = tri_mesh.calcBoundingBox();
      auto mesh = ci::gl::VboMesh::create(tri_mesh);
      panel_models[number] = mesh;
    }

    return panel_models[number];
  }


public:
  View(const ci::JsonTree& params) noexcept
    : panel_height_(params.getValueForKey<float>("panel_height")),
      put_duration_(Json::getVec<glm::vec2>(params["put_duration"])),
      put_ease_(params.getValueForKey<std::string>("put_ease"))
  {
    const auto& path = params["panel_path"];
    for (const auto p : path)
    {
      panel_path.push_back(p.getValue<std::string>());
    }
    panel_models.resize(panel_path.size());

    panel_aabb_ = ci::AxisAlignedBox(glm::vec3(-PANEL_SIZE / 2, 0, -PANEL_SIZE / 2),
                                     glm::vec3(PANEL_SIZE / 2, 1, PANEL_SIZE / 2));

    blank_model    = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("blank_model")));
    selected_model = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("selected_model")));
    cursor_model   = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("cursor_model")));
    bg_model       = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("bg_model")));
    effect_model   = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("effect_model")));
  }

  ~View() = default;


  void clear() noexcept
  {
    field_panels_.clear();
  }


  // パネル追加
  void addPanel(int index, glm::ivec2 pos, u_int rotation) noexcept
  {
    glm::ivec2 p = pos * int(PANEL_SIZE);

    static const float r_tbl[] = {
      0.0f,
      -180.0f * 0.5f,
      -180.0f,
      -180.0f * 1.5f 
    };

    Panel panel = {
      { p.x, 0, p.y },
      { 0, ci::toRadians(r_tbl[rotation]), 0 }, 
      index
    };

    field_panels_.push_back(panel);
  }

  // パネルを置く時の演出
  void startPutEase(const ci::TimelineRef& timeline, double time_rate) noexcept
  {
    auto duration = glm::mix(put_duration_.x, put_duration_.y, time_rate);

    auto& p = field_panels_.back();
    timeline->applyPtr(&p.position.y, panel_height_, 0.0f,
                       duration, getEaseFunc(put_ease_));
  }

  // 得点した時の演出
  void startEffect(const ci::TimelineRef& timeline, const glm::vec3& pos) noexcept
  {
    // ランダムに落下する立方体


  }
  

  const ci::AxisAlignedBox& panelAabb(int number) const noexcept
  {
    return panel_aabb_;
  }

  
  // パネルを１枚表示
  void drawPanel(int number, const glm::vec3& pos, u_int rotation, float rotate_offset) noexcept
  {
    static const float r_tbl[] = {
      0.0f,
      -180.0f * 0.5f,
      -180.0f,
      -180.0f * 1.5f 
    };

    ci::gl::pushModelView();
    ci::gl::translate(pos);
    ci::gl::rotate(toRadians(glm::vec3(0.0f, r_tbl[rotation] + rotate_offset, 0.0f)));
    const auto& model = getPanelModel(number);
    ci::gl::draw(model);
    ci::gl::popModelView();
  }

  void drawPanel(int number, const glm::ivec2& pos, u_int rotation) noexcept
  {
    drawPanel(number, glm::vec3(pos.x, 0.0f, pos.y), rotation, 0.0f);
  }
  
  // NOTICE 中でViewが書き換わっているのでconstにできない
  void drawFieldPanels() noexcept
  {
    ci::gl::ScopedModelMatrix m;

    const auto& panels = field_panels_;
    for (const auto& p : panels)
    {
      auto mtx = glm::translate(p.position);
      ci::gl::setModelMatrix(mtx);
      ci::gl::rotate(p.rotation);

      const auto& model = getPanelModel(p.index);
      ci::gl::draw(model);
    }
  }
  
  // Fieldの置ける場所をすべて表示
  void drawFieldBlank(const std::vector<glm::ivec2>& blank) noexcept
  {
    ci::gl::ScopedModelMatrix m;

    for (const auto& pos : blank)
    {
      glm::ivec2 p = pos * int(PANEL_SIZE);

      auto mtx = glm::translate(glm::vec3(p.x, 0.0, p.y));
      ci::gl::setModelMatrix(mtx);
      ci::gl::draw(blank_model);
    }
  }

  // 置けそうな箇所をハイライト
  void drawFieldSelected(const glm::ivec2& pos, const glm::vec3& scale) noexcept
  {
    glm::ivec2 p = pos * int(PANEL_SIZE);
    
    ci::gl::ScopedModelMatrix m;

    auto mtx = glm::translate(glm::vec3(p.x, 0.0f, p.y));
    ci::gl::setModelMatrix(mtx);
    ci::gl::scale(scale.x, scale.y, scale.z);
    ci::gl::draw(selected_model);
  }

  void drawCursor(const glm::vec3& pos, const glm::vec3& scale) noexcept
  {
    ci::gl::ScopedModelMatrix m;
    
    auto mtx = glm::translate(pos);
    ci::gl::setModelMatrix(mtx);
    ci::gl::scale(scale);
    ci::gl::draw(cursor_model);
  }

  // 背景
  void drawFieldBg(const glm::vec3& pos) noexcept
  {
    ci::gl::pushModelView();
    ci::gl::translate(pos + glm::vec3(PANEL_SIZE / 2, -15.0, PANEL_SIZE / 2));
    ci::gl::scale(PANEL_SIZE, 10.0, PANEL_SIZE);
    ci::gl::draw(bg_model);
    ci::gl::popModelView();
  }
};


#ifdef DEBUG

// パネルのエッジを表示
void drawPanelEdge(const Panel& panel, const glm::vec3& pos, u_int rotation) noexcept
{
  static const float r_tbl[] = {
    0.0f,
    -180.0f * 0.5f,
    -180.0f,
    -180.0f * 1.5f 
  };

  ci::gl::pushModelView();
  ci::gl::translate(pos.x, pos.y, pos.z);
  ci::gl::rotate(toRadians(ci::vec3(0.0f, r_tbl[rotation], 0.0f)));

  ci::gl::lineWidth(10);

  const auto& edge = panel.getEdge();
  for (auto e : edge) {
    ci::Color col;
    if (e & Panel::PATH)   col = ci::Color(1.0, 1.0, 0.0);
    if (e & Panel::FOREST) col = ci::Color(0.0, 0.5, 0.0);
    if (e & Panel::GRASS)  col = ci::Color(0.0, 1.0, 0.0);
    ci::gl::color(col);

    ci::gl::drawLine(ci::vec3(-10.1, 1, 10.1), ci::vec3(10.1, 1, 10.1));
    ci::gl::rotate(toRadians(ci::vec3(0.0f, 90.0f, 0.0f)));
  }

  ci::gl::popModelView();

  ci::gl::color(ci::Color(1, 1, 1));
}

#endif

}

