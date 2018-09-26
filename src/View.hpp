#pragma once

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
#include "Model.hpp"
#include "Shader.hpp"
#include "Utility.hpp"
#include "EaseFunc.hpp"


namespace ngs {

enum {
  PANEL_SIZE = 20,

  EFFECT_NUM     = 10,
  EFFECT_MAX_NUM = 256
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
    ci::Color color;
  };

  struct Blank
  {
    glm::ivec2 field_pos;
    glm::vec3 position;
    glm::mat4 matrix;
  };

  // Field上のパネル(Modelと被っている)
  struct Panel
  {
    glm::ivec2 field_pos;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::mat4 matrix;

    float diffuse_power;
    int index;
    int rotate_index;
    // 道や森が完成した時の演出用
    float top_y;
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

    selected_model = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("selected_model")));
    cursor_model   = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("cursor_model")));

    {
      auto size = Json::getVec<glm::ivec2>(params["shadow_map"]);
      setupShadowMap(size);
    }

    {
      auto name = params.getValueForKey<std::string>("field.shader");
      field_shader_ = createShader(name, name);

      field_shader_->uniform("uShadowMap", 0);

      field_shader_->uniform("uSpecular", Json::getColor<float>(params["field.specular"]));
      field_shader_->uniform("uShininess", params.getValueForKey<float>("field.shininess"));
      field_shader_->uniform("uAmbient", params.getValueForKey<float>("field.ambient"));
    }
    {
      blank_shader_ = createShader("blank", "blank");
      blank_shader_->uniform("uShadowMap", 0);
      blank_shader_->uniform("uSpecular", Json::getColor<float>(params["field.specular"]));
      blank_shader_->uniform("uShininess", params.getValueForKey<float>("field.shininess"));
      blank_shader_->uniform("uAmbient", params.getValueForKey<float>("field.ambient"));

      auto model = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("blank_model")));

      {
        std::vector<glm::mat4> matrix(72 * 2 + 2);
        blank_matrix_ = ci::gl::Vbo::create(GL_ARRAY_BUFFER, matrix.size() * sizeof(glm::mat4), matrix.data(), GL_DYNAMIC_DRAW);

        ci::geom::BufferLayout layout;
        layout.append(ci::geom::Attrib::CUSTOM_0, 16, sizeof(glm::mat4), 0, 1 /* per instance */);
        model->appendVbo(layout, blank_matrix_);
      }
      {
        std::vector<float> diffuse(72 * 2 + 2);
        blank_diffuse_power_ = ci::gl::Vbo::create(GL_ARRAY_BUFFER, diffuse.size() * sizeof(float), diffuse.data(), GL_DYNAMIC_DRAW);

        ci::geom::BufferLayout layout;
        layout.append(ci::geom::Attrib::CUSTOM_1, 1, sizeof(float), 0, 1 /* per instance */);
        model->appendVbo(layout, blank_diffuse_power_);
      }

      blank_model_ = ci::gl::Batch::create(model, blank_shader_,
                                           {
                                             { ci::geom::Attrib::CUSTOM_0, "vInstanceMatrix" },
                                             { ci::geom::Attrib::CUSTOM_1, "uDiffusePower" },
                                             });
    }
    {
      // 処理負荷軽減のため専用モデルを用意
      auto model = createVboMesh(params.getValueForKey<std::string>("blank_shadow_model"), false);

      ci::geom::BufferLayout layout;
      layout.append(ci::geom::Attrib::CUSTOM_0, 16, sizeof(glm::mat4), 0, 1 /* per instance */);
      model->appendVbo(layout, blank_matrix_);

      blank_shadow_shader_ = createShader("blank_shadow", "shadow");

      blank_shadow_model_ = ci::gl::Batch::create(model, blank_shadow_shader_,
                                                  {
                                                    { ci::geom::Attrib::CUSTOM_0, "vInstanceMatrix" },
                                                    });
    }
    {
      // BG
      auto name = params.getValueForKey<std::string>("bg.shader");
      bg_shader_ = createShader(name, name);
      bg_model = ci::gl::Batch::create(createVboMesh(params.getValueForKey<std::string>("bg.model"), true), bg_shader_);
      
      float checker_size = bg_scale_.x / (PANEL_SIZE / 2);
      bg_shader_->uniform("u_checker_size", checker_size);
      
      bg_shader_->uniform("u_bright", Json::getVec<glm::vec3>(params["bg.bright"]));
      bg_shader_->uniform("u_dark",   Json::getVec<glm::vec3>(params["bg.dark"]));
      bg_shader_->uniform("uTex1", 1);

      bg_shader_->uniform("uShadowMap", 0);

      bg_shader_->uniform("uSpecular", Json::getColor<float>(params["bg.specular"]));
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

      // effect_shader_->uniform("uSpecular", Json::getColor<float>(params["field.specular"]));
      // effect_shader_->uniform("uShininess", params.getValueForKey<float>("field.shininess"));
      effect_shader_->uniform("uAmbient", params.getValueForKey<float>("effect.ambient"));

      auto model = createVboMesh(params.getValueForKey<std::string>("effect.model"), true);

      {
        std::vector<glm::mat4> matrix(EFFECT_MAX_NUM);
        effect_matrix_ = ci::gl::Vbo::create(GL_ARRAY_BUFFER, matrix.size() * sizeof(glm::mat4), matrix.data(), GL_DYNAMIC_DRAW);

        ci::geom::BufferLayout layout;
        layout.append(ci::geom::Attrib::CUSTOM_0, 16, sizeof(glm::mat4), 0, 1 /* per instance */);
        model->appendVbo(layout, effect_matrix_);
      }
      {
        std::vector<ci::Color> diffuse(EFFECT_MAX_NUM);
        effect_color_ = ci::gl::Vbo::create(GL_ARRAY_BUFFER, diffuse.size() * sizeof(ci::Color), diffuse.data(), GL_DYNAMIC_DRAW);

        ci::geom::BufferLayout layout;
        layout.append(ci::geom::Attrib::CUSTOM_1, 3, sizeof(ci::Color), 0, 1 /* per instance */);
        model->appendVbo(layout, effect_color_);
      }

      effect_model_ = ci::gl::Batch::create(model, effect_shader_,
                                            {
                                              { ci::geom::Attrib::CUSTOM_0, "vInstanceMatrix" },
                                              { ci::geom::Attrib::CUSTOM_1, "uColor" },
                                              });
    }
  }

  ~View() = default;


  // Timelineとかの更新
  void update(double delta_time, bool game_paused) noexcept
  {
    put_gauge_timer_ += delta_time;
    force_timeline_->step(delta_time);
    transition_timeline_->step(delta_time);

    if (update_translate_)
    {
      updateFieldPanels();
      update_translate_ = false;
    }

    updateClouds(delta_time);

    // NOTE 以下Paush中は処理しない
    if (game_paused) return;

    timeline_->step(delta_time);
  }


  void clear() noexcept
  {
    timeline_->clear();
    force_timeline_->clear();
    // field_panels_.clear();
    // field_panel_indices_.clear();
    // blank_panels_.clear();
    effects_.clear();

    field_rotate_offset_ = 0.0f;
    update_translate_    = false;
  }

  void clearAll()
  {
    clear();
    field_panels_.clear();
    field_panel_indices_.clear();
    blank_panels_.clear();
  }


  void setColor(const ci::Color& color) noexcept
  {
    field_color_.stop();
    field_color_ = color;

    field_shader_->uniform("u_color", color);
    blank_shader_->uniform("u_color", color);
    bg_shader_->uniform("u_color", color);
    cloud_shader_->uniform("uColor", mulColor(cloud_color_, color));
    effect_shader_->uniform("u_color", color);
  }

  void setColor(float duration, const ci::Color& color, float delay = 0.0f) noexcept
  {
    auto option = transition_timeline_->apply(&field_color_, color, duration);
    option.updateFn([this]() noexcept
                    {
                      field_shader_->uniform("u_color", field_color_());
                      blank_shader_->uniform("u_color", field_color_());
                      bg_shader_->uniform("u_color", field_color_());
                      cloud_shader_->uniform("uColor", mulColor(cloud_color_, field_color_()));
                      effect_shader_->uniform("u_color", field_color_());
                    });
    option.delay(delay);
  }

  // Pause演出開始
  void pauseGame() noexcept
  {
    auto option = force_timeline_->apply(&field_rotate_offset_, toRadians(180.0f), pause_duration_.x, getEaseFunc(pause_ease_));
    option.updateFn([this]()
                    {
                      update_translate_ = true;
                    });
  }

  // Pause演出解除
  void resumeGame() noexcept
  {
    auto option = force_timeline_->apply(&field_rotate_offset_, 0.0f, pause_duration_.y, getEaseFunc(pause_ease_));
    option.updateFn([this]()
                    {
                      update_translate_ = true;
                    });
  }

  // パネル追加
  void addPanel(int index, const glm::ivec2& pos, u_int rotation) noexcept
  {
    glm::ivec2 p = pos * int(PANEL_SIZE);

    static const float r_tbl[] = {
      toRadians(0.0f),
      toRadians(-180.0f * 0.5f),
      toRadians(-180.0f),
      toRadians(-180.0f * 1.5f) 
    };

    auto yaw = r_tbl[rotation]; 
    glm::vec3 position{ p.x, 0.0f, p.y };

    auto mtx = glm::translate(position)
               * glm::eulerAngleXYZ(0.0f, yaw, 0.0f);

    // PAUSEで回転する時の適当なindex
    int rotate_index = (pos.x + pos.y * 3) & 0b11;

    Panel panel{
      pos,
      position,
      { 0.0f, yaw, 0.0f },
      mtx,
      1.0f,
      index,
      rotate_index,
      0.0f,
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

      auto delay = ci::randFloat(0.0f, 0.1f);
      options.delay(delay);
      options.updateFn([&panel]()
                       {
                         panel.matrix = glm::translate(panel.position);
                       });

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

  // パネルの行列を更新する
  void updateFieldPanels()
  {
    glm::vec2 offset[] {
      {  field_rotate_offset_, 0.0f },
      { -field_rotate_offset_, 0.0f },
      { 0.0f,  field_rotate_offset_ },
      { 0.0f, -field_rotate_offset_ },
    };

    for (auto& p : field_panels_)
    {
      const auto& ofs = offset[p.rotate_index];

      p.matrix = glm::translate(p.position)
                 * glm::eulerAngleXYZ(p.rotation.x + ofs.x, p.rotation.y, p.rotation.z + ofs.y);
    }
  }

  // パネル位置決め
  void setPanelPosition(const glm::vec3& pos) noexcept
  {
    panel_disp_ = true;

    panel_disp_pos_.stop();
    panel_disp_pos_ = pos;
  }

  // パネル移動
  void startMovePanelEase(const glm::vec3& target_pos, float rate) noexcept
  {
    auto duration = glm::mix(disp_ease_duration_.x, disp_ease_duration_.y, rate);
    timeline_->apply(&panel_disp_pos_, target_pos, duration, getEaseFunc(disp_ease_name_));
  }

  // パネル回転
  void startRotatePanelEase(float rate) noexcept
  {
    rotate_offset_ = 90.0f;
    auto duration = glm::mix(rotate_ease_duration_.x, rotate_ease_duration_.y, rate);
    timeline_->apply(&rotate_offset_, 0.0f, duration, getEaseFunc(rotate_ease_name_));
  }

  // パネルを置く時の演出
  // under  下から出現
  void startPutEase(double time_rate, bool under) noexcept
  {
    auto duration = glm::mix(put_duration_.x, put_duration_.y, time_rate);

    auto& p = field_panels_.back();
    p.position.y = under ? -15
                         : panel_height_;

    auto ease = under ? "OutBack"
                      : put_ease_;
    auto option = timeline_->applyPtr(&p.position.y, 0.0f,
                                      duration, getEaseFunc(ease));

    // 事前計算も必要なので、関数ポインタを利用
    auto func = [&p]()
                {
                  p.matrix = glm::translate(p.position)
                           * glm::eulerAngleXYZ(p.rotation.x, p.rotation.y, p.rotation.z);
                };
    func();
    option.updateFn(func);
  }

  // 次のパネルの出現演出
  void startNextPanelEase() noexcept
  {
    rotate_offset_.stop();
    rotate_offset_ = 0.0f;

    height_offset_ = height_ease_start_;
    timeline_->apply(&height_offset_, 0.0f, height_ease_duration_, getEaseFunc(height_ease_name_));
  }

  // ゲーム中断時の演出
  void abortNextPanelEase() noexcept
  {
    if (!panel_disp_) return;

    auto option = timeline_->apply(&height_offset_, -25.0f, height_ease_duration_, getEaseFunc("InQuint"));
    option.finishFn([this]()
                    {
                      rotate_offset_.stop();
                      rotate_offset_ = 0.0f;
                      panel_disp_ = false;
                    });
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
  void startEffect(const glm::ivec2& pos, float delay)
  {
    timeline_->add([this, pos]()
                   {
                     glm::vec3 gpos = vec2ToVec3(pos * int(PANEL_SIZE));

                     for (int i = 0; i < EFFECT_NUM; ++i)
                     {
                       if (effects_.size() == EFFECT_MAX_NUM) break;

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
                   },
                   timeline_->getCurrentTime() + delay);
  }

  // パネルのスケールを戻す
  void effectPanelScaing(const glm::ivec2& pos, float delay)
  {
    auto index  = field_panel_indices_.at(pos);
    auto& value = field_panels_[index].top_y;

    auto option = timeline_->applyPtr(&value, 1.0f, 0.8f, getEaseFunc("OutBack"));
    option.delay(delay);
  }


  // ゲーム終了
  void endPlay() noexcept
  {
    abortNextPanelEase();

    for (auto& panel : blank_panels_)
    {
      // Blank Panel消滅演出
      auto option = timeline_->applyPtr(&panel.position,
                                        panel.position + blank_disappear_pos_,
                                        blank_disappear_duration_, getEaseFunc(blank_disappear_ease_));

      auto delay = ci::randFloat(0.0f, 0.15f);
      option.delay(delay);
      option.updateFn([&panel]()
                      {
                        panel.matrix = glm::translate(panel.position);
                      });
    }

    timeline_->add([this]() noexcept
                   {
                     blank_panels_.clear();
                   },
                   timeline_->getCurrentTime() + blank_disappear_duration_ + 0.25f);
  }

  // Fieldのパネルをリセットする演出
  void removeFieldPanels() noexcept
  {
    glm::vec3 disappear_pos{ 0, -30, 0 };
    float duration = 0.6f;
    for (auto& panel : field_panels_)
    {
      auto option = timeline_->applyPtr(&panel.position,
                                        panel.position + disappear_pos,
                                        duration, getEaseFunc("InBack"));

      auto delay = ci::randFloat(0.0f, 0.25f);
      option.delay(delay);
      option.updateFn([this]()
                      {
                        update_translate_ = true;
                      });
    }

    timeline_->add([this]() noexcept
                   {
                     field_panels_.clear();
                     field_panel_indices_.clear();
                     // field_rotate_offset_ = 0.0f;
                   },
                   timeline_->getCurrentTime() + duration + 0.35f);
  }

  // パネルが尽きた
  void noNextPanel() noexcept
  {
    panel_disp_ = false;
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
    updateFieldBlank();

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
    cloud_shader_->uniform("uColor", mulColor(cloud_color_, field_color_()));
  }

  void setCloudAlpha(float duration, float alpha, float delay = 0.0f) noexcept
  {
    auto option = transition_timeline_->applyPtr(&cloud_color_.a, alpha, duration);
    option.updateFn([this]() noexcept
                    {
                      disp_cloud_ = cloud_color_.a > 0.0f;
                      cloud_shader_->uniform("uColor", mulColor(cloud_color_, field_color_()));
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
    blank_shader_->uniform("uSpecular", color);
    effect_shader_->uniform("uSpecular", color);
  }

  void setFieldShininess(float shininess) noexcept
  {
    field_shader_->uniform("uShininess", shininess);
    blank_shader_->uniform("uShininess", shininess);
    effect_shader_->uniform("uShininess", shininess);
  }

  void setFieldAmbient(float value) noexcept
  {
    field_shader_->uniform("uAmbient", value);
    blank_shader_->uniform("uAmbient", value);
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

  // 強制的にパネルのスケーリングを変更
  void setPanelScaling(float scale) noexcept
  {
    for (auto& panel : field_panels_)
    {
      panel.top_y = scale;
    }
  }

#endif


private:
  // 読まれてないパネルを読み込む
  const ci::gl::BatchRef& getPanelModel(int number) noexcept
  {
    if (!panel_models[number])
    {
      const auto& path = panel_path[number];
      if (!panel_model_cache_.count(path))
      {
        auto tri_mesh = Model::load(path);

        // panel_aabb[number] = tri_mesh.calcBoundingBox();
        auto mesh = ci::gl::Batch::create(tri_mesh, field_shader_);
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

    {
      ci::gl::ScopedGlslProg prog(shadow_shader_);

      drawFieldPanelShadow();
      drawFieldBlankShadow();

      if (panel_disp_)
      {
        // 手持ちパネル
        auto pos = panel_disp_pos_() + glm::vec3(0, height_offset_, 0);
        shadow_shader_->uniform("uTopY", 0.0f);
        drawPanel(info.panel_index, pos, info.panel_rotation, rotate_offset_);
      }
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

    auto mat = light_camera_.getProjectionMatrix() * light_camera_.getViewMatrix();
    field_shader_->uniform("uShadowMatrix", mat);
    blank_shader_->uniform("uShadowMatrix", mat);
    bg_shader_->uniform("uShadowMatrix", mat);

    {
      // NOTICE 点光源の位置は視野座標系に変換しとく
      auto lp = light_pos_ + info.target_pos;
      auto v = info.main_camera->getViewMatrix() * glm::vec4(lp, 1);

      field_shader_->uniform("uLightPosition", v);
      blank_shader_->uniform("uLightPosition", v);
      bg_shader_->uniform("uLightPosition", v);
      effect_shader_->uniform("uLightPosition", v);
    }

    ci::gl::ScopedGlslProg prog(field_shader_);
    ci::gl::ScopedTextureBind texScope(shadow_map_);

    drawFieldPanels();
    drawFieldBlank();

    if (panel_disp_)
    {
      field_shader_->uniform("uDiffusePower", 1.0f);
      field_shader_->uniform("uTopY", 0.0f);

      // 手持ちパネル
      auto pos = panel_disp_pos_() + glm::vec3(0, height_offset_, 0);
      drawPanel(info.panel_index, pos, info.panel_rotation, rotate_offset_);

      if (info.playing)
      {
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
    model->draw();
  }

  // Fieldのパネルを全て表示
  void drawFieldPanelShadow() noexcept
  {
    ci::gl::ScopedModelMatrix m;

    for (const auto& p : field_panels_)
    {
      ci::gl::setModelMatrix(p.matrix);

      shadow_shader_->uniform("uTopY", p.top_y);

      const auto& model = getPanelModel(p.index);
      ci::gl::draw(model->getVboMesh());
    }
  }

  void drawFieldPanels() noexcept
  {
    ci::gl::ScopedModelMatrix m;

    for (const auto& p : field_panels_)
    {
      ci::gl::setModelMatrix(p.matrix);

      field_shader_->uniform("uDiffusePower", p.diffuse_power);
      field_shader_->uniform("uTopY", p.top_y);

      const auto& model = getPanelModel(p.index);
      model->draw();
    }
  }
  
  // Fieldの置ける場所をすべて表示
  void updateFieldBlank()
  {
    if (blank_panels_.empty()) return;

    auto* mat = (glm::mat4*)blank_matrix_->mapReplace();
    auto* diffuse_power = (float*)blank_diffuse_power_->mapReplace();

    auto t = float(put_gauge_timer_ * blank_effect_speed_);
    for (const auto& p : blank_panels_)
    {
      float diffuse = glm::clamp(std::sin(t + p.position.x * blank_effect_.x + p.position.z * blank_effect_.y), 0.0f, 1.0f) * blank_diffuse_.x
                      + blank_diffuse_.y;
      *diffuse_power = diffuse;
      ++diffuse_power;

      *mat = p.matrix;
      ++mat;
    }
    blank_matrix_->unmap();
    blank_diffuse_power_->unmap();
  }


  void drawFieldBlankShadow()
  {
    if (blank_panels_.empty()) return;
    shadow_shader_->uniform("uTopY", 1.0f);
    blank_shadow_model_->drawInstanced(int(blank_panels_.size()));
  }

  void drawFieldBlank() const
  {
    if (blank_panels_.empty()) return;
    blank_model_->drawInstanced(int(blank_panels_.size()));
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
    ci::gl::ScopedTextureBind tex(bg_texture_, 1);
    ci::gl::ScopedModelMatrix m;
    glm::vec2 offset { pos.x * (1.0f / PANEL_SIZE), -pos.z * (1.0f / PANEL_SIZE) };
    bg_shader_->uniform("u_pos", offset);

    auto mtx = glm::translate(pos);
    mtx = glm::scale(mtx, bg_scale_);
    ci::gl::setModelMatrix(mtx);
    bg_model->draw();
  }

  // 演出表示
  void drawEffect() noexcept
  {
    auto* mat = (glm::mat4*)effect_matrix_->mapReplace();
    auto* color = (ci::Color*)effect_color_->mapReplace();
    int num = 0;
    for (auto it = std::begin(effects_); it != std::end(effects_); )
    {
      if (!it->active)
      {
        it = effects_.erase(it);
        continue;
      }

      if (it->disp)
      {
        *color = it->color;
        ++color;

        *mat = glm::translate(it->pos) * glm::scale(it->scale);
        ++mat;

        ++num;
      }

      ++it;
    }
    effect_matrix_->unmap();
    effect_color_->unmap();

    if (!num) return;

    effect_model_->drawInstanced(num);
  }

  // 雲
  void updateClouds(double delta_time)
  {
    for (auto& c : clouds_)
    {
      auto& pos = c.first;
      pos += c.second * float(delta_time);

      if (pos.x >  cloud_area_) pos.x -= cloud_area_ * 2;
      if (pos.x < -cloud_area_) pos.x += cloud_area_ * 2;
      if (pos.z >  cloud_area_) pos.z -= cloud_area_ * 2;
      if (pos.z < -cloud_area_) pos.z += cloud_area_ * 2;
    }
  }

  void drawClouds() const
  {
    ci::gl::ScopedGlslProg prog(cloud_shader_);
    ci::gl::ScopedTextureBind tex(cloud_texture_);
    ci::gl::ScopedModelMatrix m;

    size_t i = 0;
    for (const auto& c : clouds_)
    {
      auto mtx = glm::translate(c.first) * glm::scale(cloud_scale_);
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
  std::vector<ci::gl::BatchRef> panel_models;
  // NOTE 同じパスのモデルデータのキャッシュ
  std::map<std::string, ci::gl::BatchRef> panel_model_cache_;

  // AABBは全パネル共通
  ci::AxisAlignedBox panel_aabb_;

  // 演出用
  ci::gl::GlslProgRef blank_shader_;
  ci::gl::VboRef blank_matrix_;
  ci::gl::VboRef blank_diffuse_power_;
  ci::gl::BatchRef blank_model_;

  ci::gl::GlslProgRef blank_shadow_shader_;
  ci::gl::BatchRef blank_shadow_model_;

  ci::gl::VboMeshRef selected_model;
  ci::gl::VboMeshRef cursor_model;

  // 配置可能演出
  double blank_effect_speed_;
  glm::vec2 blank_effect_;
  glm::vec2 blank_diffuse_;

  // 背景
  ci::gl::BatchRef bg_model;
  glm::vec3 bg_scale_;

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
  ci::Anim<ci::Color> field_color_ = ci::Color::white();

  ci::gl::GlslProgRef bg_shader_;
  ci::gl::Texture2dRef bg_texture_;

  ci::gl::GlslProgRef shadow_shader_;

  // 得点時演出用
  ci::gl::GlslProgRef effect_shader_;
  ci::gl::BatchRef effect_model_;
  ci::gl::VboRef effect_matrix_;
  ci::gl::VboRef effect_color_;

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
  bool update_translate_ = false;
  glm::vec2 pause_duration_;
  std::string pause_ease_;

  // パネル位置
  bool panel_disp_ = false;
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

