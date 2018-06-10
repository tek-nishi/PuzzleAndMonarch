﻿#pragma once

//
// 表示関連
//

#include <boost/noncopyable.hpp>
#include <deque>
#include <cinder/TriMesh.h>
#include <cinder/gl/Vbo.h>
#include <cinder/gl/Batch.h>
#include <cinder/gl/Texture.h>
#include <cinder/ObjLoader.h>
#include <cinder/ImageIo.h>
#include <cinder/Timeline.h>
#include "PLY.hpp"
#include "Shader.hpp"
#include "Utility.hpp"
#include "EaseFunc.hpp"


namespace ngs {

enum {
  PANEL_SIZE = 20,
};


class View
  : private boost::noncopyable
{
  // 演出用の定義
  struct Effect
  {
    bool active;
    bool disp;
    glm::vec3 pos;
    glm::vec3 scale;
    ci::ColorA color;
  };

  struct Blank
  {
    glm::ivec2 field_pos;
    glm::vec3 position;
    glm::mat4 matrix;
  };


public:
  // 表示用の情報
  struct Info {
    bool playing;
    bool can_put;

    // 手持ちのパネル
    u_int panel_index;
    // 向き
    u_int panel_rotation;

    // 選択位置
    glm::ivec2 field_pos;
    // 背景位置
    glm::vec3 bg_pos;

    const ci::CameraPersp* main_camera;
    glm::vec3 target_pos;
  };


public:
  View(const ci::JsonTree& params) noexcept
    : polygon_offset_(Json::getVec<glm::vec2>(params["polygon_offset"])),
      panel_height_(params.getValueForKey<float>("panel_height")),
      blank_effect_speed_(params.getValueForKey<double>("blank_effect_speed")),
      blank_effect_(Json::getVec<glm::vec2>(params["blank_effect"])),
      blank_diffuse_(Json::getVec<glm::vec2>(params["blank_diffuse"])),
      bg_scale_(Json::getVec<glm::vec3>(params["bg.scale"])),
      bg_texture_(ci::gl::Texture2d::create(ci::loadImage(Asset::load(params.getValueForKey<std::string>("bg.texture"))))),
      effect_y_ofs_(Json::getVec<glm::vec2>(params["effect.y_ofs"])),
      effect_y_move_(Json::getVec<glm::vec2>(params["effect.y_move"])),
      effect_duration_(Json::getVec<glm::vec2>(params["effect.duration"])),
      effect_delay_(Json::getVec<glm::vec2>(params["effect.delay"])),
      effect_scale_(Json::getVec<glm::vec2>(params["effect.scale"])),
      effect_h_(Json::getVec<glm::vec2>(params["effect.h"])),
      effect_s_(Json::getVec<glm::vec2>(params["effect.s"])),
      effect_ease_(params.getValueForKey<std::string>("effect.ease")),
      complete_diffuse_(params.getValueForKey<float>("complete_diffuse")),
      complete_begin_duration_(params.getValueForKey<float>("complete_begin_duration")),
      complete_end_duration_(params.getValueForKey<float>("complete_end_duration")),
      complete_begin_ease_(params.getValueForKey<std::string>("complete_begin_ease")),
      complete_end_ease_(params.getValueForKey<std::string>("complete_end_ease")),
      put_duration_(Json::getVec<glm::vec2>(params["put_duration"])),
      put_ease_(params.getValueForKey<std::string>("put_ease")),
      pause_duration_(Json::getVec<glm::vec2>(params["pause_duration"])),
      pause_ease_(params.getValueForKey<std::string>("pause_ease")),
      disp_ease_duration_(Json::getVec<glm::vec2>(params["disp_ease_duration"])),
      disp_ease_name_(params.getValueForKey<std::string>("disp_ease_name")),
      rotate_ease_duration_(Json::getVec<glm::vec2>(params["rotate_ease_duration"])),
      rotate_ease_name_(params.getValueForKey<std::string>("rotate_ease_name")),
      height_ease_start_(params.getValueForKey<float>("height_ease_start")),
      height_ease_duration_(params.getValueForKey<float>("height_ease_duration")),
      height_ease_name_(params.getValueForKey<std::string>("height_ease_name")),
      blank_touch_begin_duration_(params.getValueForKey<float>("blank_touch_begin.duration")),
      blank_touch_begin_pos_(params.getValueForKey<float>("blank_touch_begin.pos")),
      blank_touch_begin_ease_(params.getValueForKey<std::string>("blank_touch_begin.ease")),
      blank_touch_cancel_duration_(params.getValueForKey<float>("blank_touch_cancel.duration")),
      blank_touch_cancel_ease_(params.getValueForKey<std::string>("blank_touch_cancel.ease")),
      blank_touch_end_duration1_(params.getValueForKey<float>("blank_touch_end.duration1")),
      blank_touch_end_pos1_(params.getValueForKey<float>("blank_touch_end.pos1")),
      blank_touch_end_ease1_(params.getValueForKey<std::string>("blank_touch_end.ease1")),
      blank_touch_end_duration2_(params.getValueForKey<float>("blank_touch_end.duration2")),
      blank_touch_end_ease2_(params.getValueForKey<std::string>("blank_touch_end.ease2")),
      blank_appear_pos_(Json::getVec<glm::vec3>(params["blank_appear.pos"])),
      blank_appear_duration_(params.getValueForKey<float>("blank_appear.duration")),
      blank_appear_ease_(params.getValueForKey<std::string>("blank_appear.ease")),
      blank_disappear_pos_(Json::getVec<glm::vec3>(params["blank_disappear.pos"])),
      blank_disappear_duration_(params.getValueForKey<float>("blank_disappear.duration")),
      blank_disappear_ease_(params.getValueForKey<std::string>("blank_disappear.ease")),
      timeline_(ci::Timeline::create()),
      force_timeline_(ci::Timeline::create()),
      transition_timeline_(ci::Timeline::create())
  {
    const auto& path = params["panel_path"];
    for (const auto p : path)
    {
      panel_path.push_back(p.getValue<std::string>());
    }
    panel_models.resize(panel_path.size());

    panel_aabb_ = ci::AxisAlignedBox(glm::vec3(-PANEL_SIZE / 2, 0, -PANEL_SIZE / 2),
                                     glm::vec3( PANEL_SIZE / 2, 2,  PANEL_SIZE / 2));

    blank_model_        = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("blank_model")));
    blank_shadow_model_ = createVboMesh(params.getValueForKey<std::string>("blank_shadow_model"), false);
    selected_model = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("selected_model")));
    cursor_model   = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("cursor_model")));
    bg_model       = createVboMesh(params.getValueForKey<std::string>("bg.model"), true);

    {
      auto size = Json::getVec<glm::ivec2>(params["shadow_map"]);
      setupShadowMap(size);
    }

    {
      auto name = params.getValueForKey<std::string>("field.shader");
      field_shader_ = createShader(name, name);

      field_shader_->uniform("uShadowMap", 0);

      field_shader_->uniform("uSpecular", Json::getColorA<float>(params["field.specular"]));
      field_shader_->uniform("uShininess", params.getValueForKey<float>("field.shininess"));
      field_shader_->uniform("uAmbient", params.getValueForKey<float>("field.ambient"));
    }
    {
      auto name = params.getValueForKey<std::string>("bg.shader");
      bg_shader_ = createShader(name, name);

      float checker_size = bg_scale_.x / (PANEL_SIZE / 2);
      bg_shader_->uniform("u_checker_size", checker_size);
      
      bg_shader_->uniform("u_bright", Json::getVec<glm::vec4>(params["bg.bright"]));
      bg_shader_->uniform("u_dark",   Json::getVec<glm::vec4>(params["bg.dark"]));
      bg_shader_->uniform("uTex1", 1);

      bg_shader_->uniform("uShadowMap", 0);

      bg_shader_->uniform("uSpecular", Json::getColorA<float>(params["bg.specular"]));
      bg_shader_->uniform("uShininess", params.getValueForKey<float>("bg.shininess"));
      bg_shader_->uniform("uAmbient", params.getValueForKey<float>("bg.ambient"));
    }

    {
      auto name = params.getValueForKey<std::string>("shadow_shader");
      shadow_shader_ = createShader(name, name);
    }
    {
      light_pos_ = Json::getVec<glm::vec3>(params["light.pos"]);
      
      float fov    = params.getValueForKey<float>("light.fov"); 
      float near_z = params.getValueForKey<float>("light.near_z"); 
      float far_z  = params.getValueForKey<float>("light.far_z"); 
      light_camera_.setPerspective(fov, shadow_fbo_->getAspectRatio(), near_z, far_z);
    }

    // 空に浮かぶ雲
    cloud_scale_ = Json::getVec<glm::vec3>(params["cloud_scale"]);
    cloud_area_  = params.getValueForKey<float>("cloud_area");
    cloud_color_ = Json::getColorA<float>(params["cloud_color"]);

    {
      // モデル準備
      std::vector<std::pair<glm::vec3, float>> cloud_bc;

      for (const auto& cloud : params["cloud_models"])
      {
        const auto& p = cloud.getValue<std::string>();

        auto tri_mesh = loadObj(p, false);
        cloud_models_.push_back(ci::gl::VboMesh::create(tri_mesh));
        auto bc = calcBoundingCircle(tri_mesh);
        bc.first  *= cloud_scale_.x;
        bc.second *= cloud_scale_.x;
        cloud_bc.push_back(bc);
      }

      // 雲のレイアウト
      layoutClouds(params, int(cloud_models_.size()), cloud_bc);
    }

    {
      auto name = params.getValueForKey<std::string>("cloud_shader");
      cloud_shader_ = createShader(name, name);
      cloud_shader_->uniform("uColor", cloud_color_);
      cloud_shader_->uniform("uThreshold", params.getValueForKey<float>("cloud_threshold"));
    }
    cloud_texture_ = ci::gl::Texture2d::create(ci::loadImage(Asset::load(params.getValueForKey<std::string>("cloud_texture"))));

    // エフェクト
    {
      auto name = params.getValueForKey<std::string>("effect.shader");
      effect_shader_ = createShader(name, name);

      effect_shader_->uniform("uSpecular", Json::getColorA<float>(params["field.specular"]));
      effect_shader_->uniform("uShininess", params.getValueForKey<float>("field.shininess"));
      effect_shader_->uniform("uAmbient", params.getValueForKey<float>("field.ambient"));
    }
    effect_model_ = createVboMesh(params.getValueForKey<std::string>("effect.model"), true);
  }

  ~View() = default;


  // Timelineとかの更新
  void update(double delta_time, bool game_paused) noexcept
  {
    put_gauge_timer_ += delta_time;
    force_timeline_->step(delta_time);
    transition_timeline_->step(delta_time);

    updateClouds(delta_time);

    if (game_paused) return;

    timeline_->step(delta_time);
  }


  void clear() noexcept
  {
    timeline_->clear();
    force_timeline_->clear();
    field_panels_.clear();
    field_panel_indices_.clear();
    blank_panels_.clear();
    effects_.clear();

    field_rotate_offset_ = 0.0f;
  }


  void setColor(const ci::ColorA& color) noexcept
  {
    field_color_.stop();
    field_color_ = color;
    field_shader_->uniform("u_color", color);
    bg_shader_->uniform("u_color", color);
    cloud_shader_->uniform("uColor", cloud_color_ * color);
    effect_shader_->uniform("u_color", color);
  }

  void setColor(float duration, const ci::ColorA& color, float delay = 0.0f) noexcept
  {
    auto option = transition_timeline_->apply(&field_color_, color, duration);
    option.updateFn([this]() noexcept
                    {
                      field_shader_->uniform("u_color", field_color_());
                      bg_shader_->uniform("u_color", field_color_());
                      cloud_shader_->uniform("uColor", cloud_color_ * field_color_());
                      effect_shader_->uniform("u_color", field_color_());
                    });
    option.delay(delay);
  }

  // Pause演出開始
  void pauseGame() noexcept
  {
    force_timeline_->apply(&field_rotate_offset_, toRadians(180.0f), pause_duration_.x, getEaseFunc(pause_ease_));
  }

  // Pause演出解除
  void resumeGame() noexcept
  {
    force_timeline_->apply(&field_rotate_offset_, 0.0f, pause_duration_.y, getEaseFunc(pause_ease_));
  }

  // パネル追加
  void addPanel(int index, const glm::ivec2& pos, u_int rotation) noexcept
  {
    glm::ivec2 p = pos * int(PANEL_SIZE);

    static const float r_tbl[] = {
      0.0f,
      -180.0f * 0.5f,
      -180.0f,
      -180.0f * 1.5f 
    };

    Panel panel = {
      pos,
      { p.x, 0, p.y },
      { 0, ci::toRadians(r_tbl[rotation]), 0 },
      1.0f,
      index
    };

    field_panel_indices_.insert({ pos, field_panels_.size() });
    field_panels_.push_back(panel);
  }

  // Blank更新
  void updateBlank(const std::vector<glm::ivec2>& blanks) noexcept
  {
    // 新しいのを追加
    for (const auto& pos : blanks)
    {
      if (searchBlank(pos)) continue; 

      glm::vec3 blank_pos = vec2ToVec3(pos * int(PANEL_SIZE));
      blank_panels_.push_back({ pos, blank_pos });

      // Blank Panel出現演出
      auto& panel    = blank_panels_.back();
      auto panel_pos = panel.position + blank_appear_pos_;
      auto options   = timeline_->applyPtr(&panel.position,
                                           panel_pos, panel.position,
                                           blank_appear_duration_, getEaseFunc(blank_appear_ease_));
      options.updateFn([&panel]()
                       {
                         panel.matrix = glm::translate(panel.position);
                       });

      // panel.position = panel_pos;
      panel.matrix   = glm::translate(panel_pos);
    }

    for (auto it = std::begin(blank_panels_); it != std::end(blank_panels_); )
    {
      if (existsBlank(it->field_pos, blanks))
      {
        ++it;
        continue;
      }

      timeline_->removeTarget(&it->position);
      it = blank_panels_.erase(it);
    }
  }

  // パネル位置決め
  void setPanelPosition(const glm::vec3& pos) noexcept
  {
    panel_disp_pos_.stop();
    panel_disp_pos_ = pos;
  }

  // パネル移動
  void startMovePanelEase(const glm::vec3& target_pos, float rate) noexcept
  {
    // panel_disp_pos_.stop();
    auto duration = glm::mix(disp_ease_duration_.x, disp_ease_duration_.y, rate);
    timeline_->apply(&panel_disp_pos_, target_pos, duration, getEaseFunc(disp_ease_name_));
  }

  // パネル回転
  void startRotatePanelEase(float rate) noexcept
  {
    // rotate_offset_.stop();
    rotate_offset_ = 90.0f;
    auto duration = glm::mix(rotate_ease_duration_.x, rotate_ease_duration_.y, rate);
    timeline_->apply(&rotate_offset_, 0.0f, duration, getEaseFunc(rotate_ease_name_));
  }

  // パネルを置く時の演出
  void startPutEase(double time_rate) noexcept
  {
    auto duration = glm::mix(put_duration_.x, put_duration_.y, time_rate);

    auto& p = field_panels_.back();
    p.position.y = panel_height_;
    timeline_->applyPtr(&p.position.y, 0.0f,
                        duration, getEaseFunc(put_ease_));
  }

  // 次のパネルの出現演出
  void startNextPanelEase() noexcept
  {
    rotate_offset_.stop();
    rotate_offset_ = 0.0f;

    height_offset_ = height_ease_start_;
    timeline_->apply(&height_offset_, 0.0f, height_ease_duration_, getEaseFunc(height_ease_name_));
  }

  // Blankをタッチした時の演出
  void blankTouchBeginEase(const glm::vec2& pos) noexcept
  {
    auto* p = searchBlankPosition(pos);
    if (!p)
    {
      DOUT << "No blank position:" << pos << std::endl;
      return;
    }

    auto option = timeline_->applyPtr(&p->position.y, 0.0f, blank_touch_begin_pos_,
                                      blank_touch_begin_duration_, getEaseFunc(blank_touch_begin_ease_));
    option.updateFn([p]()
                    {
                      p->matrix = glm::translate(p->position);
                    });
  }

  // Blankのタッチをやめた時の演出
  void blankTouchEndEase(const glm::vec2& pos) noexcept
  {
    auto* p = searchBlankPosition(pos);
    if (!p)
    {
      DOUT << "No blank position:" << pos << std::endl;
      return;
    }

    {
      auto option = timeline_->applyPtr(&p->position.y, blank_touch_end_pos1_,
                                        blank_touch_end_duration1_, getEaseFunc(blank_touch_end_ease1_));
      option.updateFn([p]()
                      {
                        p->matrix = glm::translate(p->position);
                      });
    }
    {
      auto option = timeline_->appendToPtr(&p->position.y, 0.0f,
                                           blank_touch_end_duration2_, getEaseFunc(blank_touch_end_ease2_));
      option.updateFn([p]()
                      {
                        p->matrix = glm::translate(p->position);
                      });
    }
  }

  void blankTouchCancelEase(const glm::vec2& pos) noexcept
  {
    auto* p = searchBlankPosition(pos);
    if (!p)
    {
      DOUT << "No blank position:" << pos << std::endl;
      return;
    }

    auto option = timeline_->applyPtr(&p->position.y, 0.0f,
                                 blank_touch_cancel_duration_, getEaseFunc(blank_touch_cancel_ease_));
    option.updateFn([p]()
                    {
                      p->matrix = glm::translate(p->position);
                    });
  }

  // 得点した時の演出
  void startEffect(const glm::ivec2& pos) noexcept
  {
    glm::vec3 gpos = vec2ToVec3(pos * int(PANEL_SIZE));

    for (int i = 0; i < 10; ++i)
    {
      glm::vec3 ofs{
        ci::randFloat(-PANEL_SIZE / 2, PANEL_SIZE / 2),
        randFromVec2(effect_y_ofs_),
        ci::randFloat(-PANEL_SIZE / 2, PANEL_SIZE / 2)
      };
      effects_.push_back({ true, false, gpos + ofs });
      auto& effect = effects_.back();

      auto end_pos = gpos + ofs + glm::vec3(0, randFromVec2(effect_y_move_), 0);

      float duration = randFromVec2(effect_duration_);
      float delay    = randFromVec2(effect_delay_);

      auto options = timeline_->applyPtr(&effect.pos, end_pos, duration, getEaseFunc(effect_ease_));

      options.delay(delay);
      options.startFn([&effect, this]() noexcept
                      {
                        effect.disp  = true;
                        effect.scale = glm::vec3(randFromVec2(effect_scale_));
                        glm::vec3 hsv{
                          randFromVec2(effect_h_), 
                          randFromVec2(effect_s_),
                          1.0f
                        };
                        effect.color = ci::hsvToRgb(hsv);
                      });
      options.finishFn([&effect]() noexcept
                       {
                         effect.active = false;
                       });
    }

    // パネル発光演出
    if (field_panel_indices_.count(pos))
    {
      auto index  = field_panel_indices_.at(pos);
      auto& value = field_panels_[index].diffuse_power;

      timeline_->applyPtr(&value, complete_diffuse_, complete_begin_duration_, getEaseFunc(complete_begin_ease_));
      timeline_->appendToPtr(&value, 1.0f, complete_end_duration_, getEaseFunc(complete_end_ease_));
    }
  }


  // ゲーム終了
  void endPlay() noexcept
  {
    for (auto& panel : blank_panels_)
    {
      // Blank Panel消滅演出
      timeline_->applyPtr(&panel.position,
                          panel.position + blank_disappear_pos_,
                          blank_disappear_duration_, getEaseFunc(blank_disappear_ease_));
    }

    timeline_->add([this]() noexcept
                   {
                     blank_panels_.clear();
                   },
                   timeline_->getCurrentTime() + blank_disappear_duration_ + 0.1f);
  }


  const ci::AxisAlignedBox& panelAabb(int number) const noexcept
  {
    return panel_aabb_;
  }

  // ShadowMap用のカメラ更新
  void setupShadowCamera(const glm::vec3& map_center) noexcept
  {
    light_camera_.lookAt(map_center + light_pos_, map_center);
  }

  // フィールド表示
  void drawField(const Info& info) noexcept
  {
    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);
    ci::gl::disableAlphaBlending();

    renderShadow(info);
    renderField(info);
  }


  void toggleCloud() noexcept
  {
    disp_cloud_ = !disp_cloud_;
    if (disp_cloud_)
    {
      setCloudAlpha(1.0f);
    }
  }

  void setCloudAlpha(float alpha) noexcept
  {
    transition_timeline_->removeTarget(&cloud_color_.a);

    disp_cloud_ = alpha > 0.0f;
    cloud_color_.a = alpha;
    cloud_shader_->uniform("uColor", cloud_color_ * field_color_());
  }

  void setCloudAlpha(float duration, float alpha, float delay = 0.0f) noexcept
  {
    auto option = transition_timeline_->applyPtr(&cloud_color_.a, alpha, duration);
    option.updateFn([this]() noexcept
                    {
                      disp_cloud_ = cloud_color_.a > 0.0f;
                      cloud_shader_->uniform("uColor", cloud_color_ * field_color_());
                    });
    option.delay(delay);
  }


#if defined (DEBUG)

  void drawShadowMap() noexcept
  {
    ci::gl::setMatricesWindow(ci::app::getWindowSize());
    ci::gl::color(1.0f, 1.0f, 1.0f);
    float size = 0.5f * std::min(ci::app::getWindowWidth(), ci::app::getWindowHeight());
    ci::gl::draw(shadow_map_, ci::Rectf(0, 0, size, size));
  }
  
  void setPolygonFactor(float value) noexcept
  {
    polygon_offset_.x = value;
  }

  void setPolygonUnits(float value) noexcept
  {
    polygon_offset_.y = value;
  }


  void setFieldSpecular(const ci::ColorA& color) noexcept
  {
    field_shader_->uniform("uSpecular", color);
    effect_shader_->uniform("uSpecular", color);
  }

  void setFieldShininess(float shininess) noexcept
  {
    field_shader_->uniform("uShininess", shininess);
    effect_shader_->uniform("uShininess", shininess);
  }

  void setFieldAmbient(float value) noexcept
  {
    field_shader_->uniform("uAmbient", value);
    effect_shader_->uniform("uAmbient", value);
  }

  void setBgSpecular(const ci::ColorA& color) noexcept
  {
    bg_shader_->uniform("uSpecular", color);
  }

  void setBgShininess(float shininess) noexcept
  {
    bg_shader_->uniform("uShininess", shininess);
  }

  void setBgAmbient(float value) noexcept
  {
    bg_shader_->uniform("uAmbient", value);
  }
  

  void setLightPosition(const glm::vec3& position) noexcept
  {
    light_pos_ = position;
    setupShadowCamera(glm::vec3());
  }

#endif


private:
  // 読まれてないパネルを読み込む
  const ci::gl::VboMeshRef& getPanelModel(int number) noexcept
  {
    if (!panel_models[number])
    {
      const auto& path = panel_path[number];
      if (!panel_model_cache_.count(path))
      {
        auto tri_mesh = PLY::load(path);
        // panel_aabb[number] = tri_mesh.calcBoundingBox();
        auto mesh = ci::gl::VboMesh::create(tri_mesh);
        panel_models[number] = mesh;
        panel_model_cache_.insert({ path, mesh });
      }
      else
      {
        DOUT << "Use cache: " << path << std::endl;
        panel_models[number] = panel_model_cache_.at(path);
      }
    }

    return panel_models[number];
  }


  // OBJ形式→TriMesh
  static ci::TriMesh loadObj(const std::string& path, bool has_normal)
  {
    ci::ObjLoader loader{ Asset::load(path) };

    auto format = ci::TriMesh::Format().positions().texCoords();
    if (has_normal) format.normals();

    auto mesh = ci::TriMesh{ loader, format };

    // DOUT << path
    // << ": N: " << mesh.hasNormals()
    // << " C: "  << mesh.hasColors()
    // << " T: "  << mesh.hasTexCoords()
    // << std::endl;

    return mesh;
  }

  // OBJ形式からVboMeshを生成
  static ci::gl::VboMeshRef createVboMesh(const std::string& path, bool has_normal)
  {
    return ci::gl::VboMesh::create(loadObj(path, has_normal));
  }

  // TriMeshの外接円をなんとなく求める
  static std::pair<glm::vec3, float> calcBoundingCircle(const ci::TriMesh& mesh)
  {
    auto aabb = mesh.calcBoundingBox();
    const auto& center  = aabb.getCenter();
    const auto& extents = aabb.getExtents();

    float radius = std::max(extents.x, extents.y);

    DOUT << "center:" << center
         << " extents:" << extents
         << " radius: " << radius 
         << std::endl;

    return { center, radius };
  }


  // 影レンダリング用の設定
  void setupShadowMap(const glm::ivec2& fbo_size) noexcept
  {
    ci::gl::Texture2d::Format depthFormat;

    depthFormat.internalFormat(GL_DEPTH_COMPONENT16)
               .compareMode(GL_COMPARE_REF_TO_TEXTURE)
               .magFilter(GL_LINEAR)
               .minFilter(GL_LINEAR)
               .wrap(GL_REPEAT)	
               .compareFunc(GL_LEQUAL)
    ;

    shadow_map_ = ci::gl::Texture2d::create(fbo_size.x, fbo_size.y, depthFormat);

    try
    {
      ci::gl::Fbo::Format fboFormat;
      fboFormat.attachment(GL_DEPTH_ATTACHMENT, shadow_map_)
               .disableColor()
      ;
      shadow_fbo_ = ci::gl::Fbo::create(fbo_size.x, fbo_size.y, fboFormat);
    }
    catch (const std::exception& e)
    {
      DOUT << "FBO ERROR: " << e.what() << std::endl;
    }
  }

  // Blank Panel関連
  bool searchBlank(const glm::ivec2& pos) noexcept
  {
    for (const auto& b : blank_panels_)
    {
      if (b.field_pos == pos) return true;
    }
    return false;
  }

  Blank* searchBlankPosition(const glm::ivec2& pos) noexcept
  {
    for (auto& b : blank_panels_)
    {
      if (b.field_pos == pos) return &b;
    }
    return nullptr;
  }

  bool existsBlank(const glm::ivec2& pos, const std::vector<glm::ivec2>& blanks) noexcept
  {
    for (const auto& p : blanks)
    {
      if (p == pos) return true;
    }

    return false;
  }

  // 影のレンダリング
  void renderShadow(const Info& info) noexcept
  {
    // Set polygon offset to battle shadow acne
    ci::gl::enable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(polygon_offset_.x, polygon_offset_.y);

    // Render scene to fbo from the view of the light
    ci::gl::ScopedFramebuffer fbo(shadow_fbo_);
    ci::gl::ScopedViewport viewport(glm::vec2(), shadow_fbo_->getSize());
    ci::gl::clear(GL_DEPTH_BUFFER_BIT);
    ci::gl::setMatrices(light_camera_);

    ci::gl::ScopedGlslProg prog(shadow_shader_);

    drawFieldPanels(false);
    drawFieldBlankShadow();

    if (info.playing)
    {
      // 手持ちパネル
      auto pos = panel_disp_pos_() + glm::vec3(0, height_offset_, 0);
      drawPanel(info.panel_index, pos, info.panel_rotation, rotate_offset_);
    }

    // 雲
    drawClouds();

    // Disable polygon offset for final render
    ci::gl::disable(GL_POLYGON_OFFSET_FILL);
  }

  // Field描画
  void renderField(const Info& info) noexcept
  {
    ci::gl::setMatrices(*info.main_camera);
    ci::gl::clear(ci::Color::black());

    auto mat = light_camera_.getProjectionMatrix() * light_camera_.getViewMatrix();
    field_shader_->uniform("uShadowMatrix", mat);
    bg_shader_->uniform("uShadowMatrix", mat);

    {
      // NOTICE 点光源の位置は視野座標系に変換しとく
      auto v = info.main_camera->getViewMatrix() * glm::vec4(light_pos_, 1);

      field_shader_->uniform("uLightPosition", v);
      bg_shader_->uniform("uLightPosition", v);
      effect_shader_->uniform("uLightPosition", v);
    }

    ci::gl::ScopedGlslProg prog(field_shader_);
    ci::gl::ScopedTextureBind texScope(shadow_map_);

    drawFieldPanels(true);
    drawFieldBlank();

    if (info.playing)
    {
      field_shader_->uniform("uDiffusePower", 1.0f);

      // 手持ちパネル
      auto pos = panel_disp_pos_() + glm::vec3(0, height_offset_, 0);
      drawPanel(info.panel_index, pos, info.panel_rotation, rotate_offset_);

      // 選択箇所
      float s = std::abs(std::sin(put_gauge_timer_ * 6.0)) * 0.1;
      glm::vec3 scale(0.9 + s, 1, 0.9 + s);
      drawFieldSelected(info.field_pos, scale);

      // 「置けますよ」アピール
      if (info.can_put)
      {
        scale.x = 1.0 + s;
        scale.z = 1.0 + s;
        drawCursor(pos, scale);
      }
    }

    drawFieldBg(info.bg_pos);
    drawEffect();

    if (disp_cloud_)
    {
      ci::gl::enableDepth(false);
      ci::gl::disable(GL_CULL_FACE);
      ci::gl::enableAlphaBlending();

      drawClouds();
    }
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
    
    ci::gl::ScopedModelMatrix m;

    auto mtx = glm::translate(pos) * glm::eulerAngleXYZ(0.0f, toRadians(r_tbl[rotation] + rotate_offset), 0.0f);
    ci::gl::setModelMatrix(mtx);

    const auto& model = getPanelModel(number);
    ci::gl::draw(model);
  }

  // Fieldのパネルを全て表示
  void drawFieldPanels(bool diffuse) noexcept
  {
    ci::gl::ScopedModelMatrix m;

    glm::vec2 offset[] = {
      {  field_rotate_offset_, 0.0f },
      { -field_rotate_offset_, 0.0f },
      { 0.0f,  field_rotate_offset_ },
      { 0.0f, -field_rotate_offset_ },
    };

    const auto& panels = field_panels_;
    for (const auto& p : panels)
    {
      // TODO この計算はaddの時に済ませる
      int index = (p.field_pos.x + p.field_pos.y * 3) & 0b11;
      const auto& ofs = offset[index];

      auto mtx = glm::translate(p.position)
                 * glm::eulerAngleXYZ(p.rotation.x + ofs.x, p.rotation.y, p.rotation.z + ofs.y);
      ci::gl::setModelMatrix(mtx);

      if (diffuse)
      {
        field_shader_->uniform("uDiffusePower", p.diffuse_power);
      }

      const auto& model = getPanelModel(p.index);
      ci::gl::draw(model);
    }
  }
  
  // Fieldの置ける場所をすべて表示
  void drawFieldBlankShadow() const
  {
    ci::gl::ScopedModelMatrix m;

    for (const auto& p : blank_panels_)
    {
      ci::gl::setModelMatrix(p.matrix);
      ci::gl::draw(blank_shadow_model_);
    }
  }

  void drawFieldBlank() const
  {
    ci::gl::ScopedModelMatrix m;

    auto t = float(put_gauge_timer_ * blank_effect_speed_);
    for (const auto& p : blank_panels_)
    {
      float diffuse = glm::clamp(std::sin(t + p.position.x * blank_effect_.x + p.position.z * blank_effect_.y), 0.0f, 1.0f)
                      * blank_diffuse_.x + blank_diffuse_.y;
      field_shader_->uniform("uDiffusePower", diffuse);

      ci::gl::setModelMatrix(p.matrix);
      ci::gl::draw(blank_model_);
    }
  }

  // 置けそうな箇所をハイライト
  void drawFieldSelected(const glm::ivec2& pos, const glm::vec3& scale) noexcept
  {
    ci::gl::ScopedModelMatrix m;

    auto mtx = glm::translate(vec2ToVec3(pos * int(PANEL_SIZE)));
    mtx = glm::scale(mtx, scale);
    ci::gl::setModelMatrix(mtx);
    ci::gl::draw(selected_model);
  }

  void drawCursor(const glm::vec3& pos, const glm::vec3& scale) noexcept
  {
    ci::gl::ScopedModelMatrix m;
    
    auto mtx = glm::translate(pos);
    mtx = glm::scale(mtx, scale);
    ci::gl::setModelMatrix(mtx);
    ci::gl::draw(cursor_model);
  }

  // 背景
  void drawFieldBg(const glm::vec3& pos) noexcept
  {
    ci::gl::ScopedGlslProg prog(bg_shader_);
    ci::gl::ScopedTextureBind tex(bg_texture_, 1);
    ci::gl::ScopedModelMatrix m;
    glm::vec2 offset { pos.x * (1.0f / PANEL_SIZE), -pos.z * (1.0f / PANEL_SIZE) };
    bg_shader_->uniform("u_pos", offset);

    auto mtx = glm::translate(pos);
    mtx = glm::scale(mtx, bg_scale_);
    ci::gl::setModelMatrix(mtx);
    ci::gl::draw(bg_model);
  }

  // 演出表示
  void drawEffect() noexcept
  {
    ci::gl::ScopedGlslProg prog(effect_shader_);
    ci::gl::ScopedModelMatrix m;

    for (auto it = std::begin(effects_); it != std::end(effects_); )
    {
      if (!it->active)
      {
        it = effects_.erase(it);
        continue;
      }

      if (it->disp)
      {
        effect_shader_->uniform("uColor", it->color);

        auto mtx = glm::translate(it->pos) * glm::scale(it->scale);
        ci::gl::setModelMatrix(mtx);
        ci::gl::draw(effect_model_);
      }

      ++it;
    }
  }

  // 雲
  void updateClouds(double delta_time)
  {
    for (auto& c : clouds_)
    {
      auto& pos = c.first;
      pos += c.second * float(delta_time);
    }
  }

  void drawClouds()
  {
    ci::gl::ScopedGlslProg prog(cloud_shader_);
    ci::gl::ScopedTextureBind tex(cloud_texture_);
    ci::gl::ScopedModelMatrix m;

    size_t i = 0;
    for (auto& c : clouds_)
    {
      auto& pos = c.first;

      if (pos.x >  cloud_area_) pos.x -= cloud_area_ * 2;
      if (pos.x < -cloud_area_) pos.x += cloud_area_ * 2;
      if (pos.z >  cloud_area_) pos.z -= cloud_area_ * 2;
      if (pos.z < -cloud_area_) pos.z += cloud_area_ * 2;

      auto mtx = glm::translate(pos) * glm::scale(cloud_scale_);
      ci::gl::setModelMatrix(mtx);
      ci::gl::draw(cloud_models_[i % cloud_models_.size()]);
      ++i;
    }
  }

  // 雲の配置
  //   なるべく均等に配置されるようにする
  void layoutClouds(const ci::JsonTree& params,
                    int cloud_kinds,
                    const std::vector<std::pair<glm::vec3, float>>& bounding_circle) noexcept
  {
    auto x_pos = Json::getVec<glm::vec2>(params["cloud_pos"][0]);
    auto y_pos = Json::getVec<glm::vec2>(params["cloud_pos"][1]);
    auto z_pos = Json::getVec<glm::vec2>(params["cloud_pos"][2]);

    auto dir   = glm::normalize(Json::getVec<glm::vec3>(params["cloud_dir"]));
    auto speed = Json::getVec<glm::vec2>(params["cloud_speed"]);

    auto num = params.getValueForKey<int>("cloud_num");
    for (int i = 0; i < num; ++i)
    {
      glm::vec3 p{
        randFromVec2(x_pos),
        randFromVec2(y_pos),
        randFromVec2(z_pos)
      };
      auto v = dir * randFromVec2(speed);

      clouds_.push_back({ p, v });
    }

    // 総当たりで相互の距離を調整する
    for (int i = 0; i < clouds_.size(); ++i)
    {
      const auto& p1  = clouds_[i].first;
      const auto& bc1 = bounding_circle[i % cloud_kinds];
      auto pos1 = p1 + bc1.first;
      float r1  = bc1.second;

      for (int j = 0; j < clouds_.size(); ++j)
      {
        if (i == j) continue;

        auto& p2  = clouds_[j].first;
        const auto& bc2 = bounding_circle[j % cloud_kinds];
        auto pos2 = p2 + bc2.first;
        float r2  = bc2.second;

        auto d = glm::distance2(pos1, pos2);
        if (d < (r1 + r2) * (r1 + r2))
        {
          if (d > 0.0f)
          {
            // 重ならない位置へ移動
            auto v = glm::normalize(pos2 - pos1);
            auto np2 = pos1 + v * (r1 + r2);
            p2 = np2 - bc2.first;
          }
          else
          {
            // 完全に重なっていた場合
            auto np2 = pos1 + glm::vec3{ r1 + r2, 0.0f, 0.0f };
            p2 = np2 - bc2.first;
          }
        }
      }
    }
  }


  // パネル
  std::vector<std::string> panel_path;
  std::vector<ci::gl::VboMeshRef> panel_models;
  // NOTE 同じパスのモデルデータのキャッシュ
  std::map<std::string, ci::gl::VboMeshRef> panel_model_cache_;

  // AABBは全パネル共通
  ci::AxisAlignedBox panel_aabb_;

  // 演出用
  ci::gl::VboMeshRef blank_model_;
  ci::gl::VboMeshRef blank_shadow_model_;
  ci::gl::VboMeshRef selected_model;
  ci::gl::VboMeshRef cursor_model;

  // 配置可能演出
  double blank_effect_speed_;
  glm::vec2 blank_effect_;
  glm::vec2 blank_diffuse_;

  // 背景
  ci::gl::VboMeshRef bg_model;
  glm::vec3 bg_scale_;

  // Field上のパネル(Modelと被っている)
  struct Panel
  {
    glm::ivec2 field_pos;
    glm::vec3 position;
    glm::vec3 rotation;
    float diffuse_power;
    int index;
  };
  // NOTICE 追加時にメモリ上で再配置されるのを避けるためstd::vectorではない
  std::deque<Panel> field_panels_;
  // 光る演出用
  std::map<glm::ivec2, size_t, LessVec<glm::ivec2>> field_panel_indices_;

  // 光る演出タイミング
  float complete_diffuse_;
  float complete_begin_duration_;
  float complete_end_duration_;
  std::string complete_begin_ease_;
  std::string complete_end_ease_;

  std::list<Blank> blank_panels_;

  ci::gl::GlslProgRef field_shader_;
  ci::Anim<ci::ColorA> field_color_ = ci::ColorA::white();

  ci::gl::GlslProgRef bg_shader_;
  ci::gl::Texture2dRef bg_texture_;

  ci::gl::GlslProgRef shadow_shader_;


  // 得点時演出用
  ci::gl::VboMeshRef effect_model_;
  ci::gl::GlslProgRef effect_shader_;
  glm::vec2 effect_y_ofs_;
  glm::vec2 effect_y_move_;
  glm::vec2 effect_delay_;
  glm::vec2 effect_duration_;
  glm::vec2 effect_scale_;
  glm::vec2 effect_h_;
  glm::vec2 effect_s_;
  std::string effect_ease_;

  // 影レンダリング用
  ci::gl::Texture2dRef shadow_map_;
  ci::gl::FboRef shadow_fbo_;

  glm::vec2 polygon_offset_;

  ci::CameraPersp light_camera_;
  glm::vec3 light_pos_;

  // 画面演出用情報
  float panel_height_;
  glm::vec2 put_duration_;
  std::string put_ease_;

  // PAUSE時にくるっと回す用
  ci::Anim<float> field_rotate_offset_ = 0.0f;
  glm::vec2 pause_duration_;
  std::string pause_ease_;

  // パネル位置
  ci::Anim<glm::vec3> panel_disp_pos_;
  glm::vec2 disp_ease_duration_;
  std::string disp_ease_name_;

  // パネルの回転
  ci::Anim<float> rotate_offset_;
  glm::vec2 rotate_ease_duration_;
  std::string rotate_ease_name_;

  // 次のパネルを引いた時の演出
  ci::Anim<float> height_offset_;
  float height_ease_start_;
  float height_ease_duration_;
  std::string height_ease_name_;

  // Blankタッチ演出
  float blank_touch_begin_duration_;
  float blank_touch_begin_pos_;
  std::string blank_touch_begin_ease_;

  float blank_touch_cancel_duration_;
  std::string blank_touch_cancel_ease_;

  float blank_touch_end_duration1_;
  float blank_touch_end_pos1_;
  std::string blank_touch_end_ease1_;

  float blank_touch_end_duration2_;
  std::string blank_touch_end_ease2_;

  // Blank出現演出
  glm::vec3 blank_appear_pos_;
  float blank_appear_duration_;
  std::string blank_appear_ease_;

  glm::vec3 blank_disappear_pos_;
  float blank_disappear_duration_;
  std::string blank_disappear_ease_;

  // パネルを置くゲージ演出
  double put_gauge_timer_ = 0.0;

  // FIXME 途中の削除が多いのでvectorは向いていない??
  std::list<Effect> effects_;

  // 雲演出
  std::vector<ci::gl::VboMeshRef> cloud_models_;
  ci::gl::Texture2dRef cloud_texture_;
  ci::gl::GlslProgRef cloud_shader_;
  std::vector<std::pair<glm::vec3, glm::vec3>> clouds_;
  glm::vec3 cloud_scale_;
  float cloud_area_;
  ci::ColorA cloud_color_;
  bool disp_cloud_ = true;


  // Tween用
  ci::TimelineRef timeline_;
  // Pause中でも動作
  ci::TimelineRef force_timeline_;
  // リセットされたくない
  ci::TimelineRef transition_timeline_;
};

}

