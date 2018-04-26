#pragma once

//
// 表示関連
//

#include <boost/noncopyable.hpp>
#include <deque>
#include <cinder/TriMesh.h>
#include <cinder/gl/Vbo.h>
#include <cinder/gl/Texture.h>
#include <cinder/ObjLoader.h>
#include <cinder/ImageIo.h>
#include "PLY.hpp"
#include "Shader.hpp"


namespace ngs {

enum {
  PANEL_SIZE = 20,
};


class View
  : private boost::noncopyable
{
  // 演出用の定義
  struct Effect {
    bool active;
    glm::vec3 pos;
    glm::vec3 rot;
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
  };


public:
  View(const ci::JsonTree& params) noexcept
    : polygon_offset_(Json::getVec<glm::vec2>(params["polygon_offset"])),
      panel_height_(params.getValueForKey<float>("panel_height")),
      bg_scale_(Json::getVec<glm::vec3>(params["bg_scale"])),
      bg_texture_(ci::gl::Texture2d::create(ci::loadImage(Asset::load(params.getValueForKey<std::string>("bg_texture"))))),
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
                                     glm::vec3(PANEL_SIZE / 2, 1, PANEL_SIZE / 2));

    blank_model    = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("blank_model")));
    selected_model = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("selected_model")));
    cursor_model   = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("cursor_model")));
    bg_model       = loadObj(params.getValueForKey<std::string>("bg_model"));
    effect_model   = ci::gl::VboMesh::create(PLY::load(params.getValueForKey<std::string>("effect_model")));

    {
      auto size = Json::getVec<glm::ivec2>(params["shadow_map"]);
      setupShadowMap(size);
    }

    {
      auto name = params.getValueForKey<std::string>("field_shader");
      field_shader_ = createShader(name, name);

      field_shader_->uniform("uShadowMap", 0);
      field_shader_->uniform("uShadowIntensity", params.getValueForKey<float>("shadow_intensity"));
    }
    {
      auto name = params.getValueForKey<std::string>("bg_shader");
      bg_shader_ = createShader(name, name);

      float checker_size = bg_scale_.x / (PANEL_SIZE / 2);
      bg_shader_->uniform("u_checker_size", checker_size);
      
      bg_shader_->uniform("u_bright", Json::getVec<glm::vec4>(params["bg_bright"]));
      bg_shader_->uniform("u_dark",   Json::getVec<glm::vec4>(params["bg_dark"]));
      bg_shader_->uniform("uShadowMap", 0);
      bg_shader_->uniform("uTex1", 1);
      bg_shader_->uniform("uShadowIntensity", params.getValueForKey<float>("shadow_intensity"));
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

    for (const auto& cloud : params["cloud_models"])
    {
      const auto& p = cloud.getValue<std::string>();
      cloud_models_.push_back(loadObj(p));
    }

    {
      auto x_pos = Json::getVec<glm::vec2>(params["cloud_pos"][0]);
      auto y_pos = Json::getVec<glm::vec2>(params["cloud_pos"][1]);
      auto z_pos = Json::getVec<glm::vec2>(params["cloud_pos"][2]);

      auto dir   = glm::normalize(Json::getVec<glm::vec3>(params["cloud_dir"]));
      auto speed = Json::getVec<glm::vec2>(params["cloud_speed"]);

      auto num = params.getValueForKey<int>("cloud_num");
      for (int i = 0; i < num; ++i)
      {
        glm::vec3 p { ci::randFloat(x_pos.x, x_pos.y),
                      ci::randFloat(y_pos.x, y_pos.y),
                      ci::randFloat(z_pos.x, z_pos.y) };
        auto v = dir * ci::randFloat(speed.x, speed.y);

        clouds_.push_back({ p, v });
      }

      cloud_scale_ = Json::getVec<glm::vec3>(params["cloud_scale"]);
      cloud_area_  = params.getValueForKey<float>("cloud_area");
    }
    {
      auto name = params.getValueForKey<std::string>("cloud_shader");
      cloud_shader_ = createShader(name, name);
    }
    {
      auto format = ci::gl::Texture2d::Format();   // .minFilter(GL_NEAREST).magFilter(GL_NEAREST);
      cloud_texture_ = ci::gl::Texture2d::create(ci::loadImage(Asset::load(params.getValueForKey<std::string>("cloud_texture"))),
                                                 format);
    }

#if defined (DEBUG)
    disp_cloud_ = Json::getValue(params, "cloud_disp", false);
#endif
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
  }

  void setColor(float duration, const ci::ColorA& color, float delay = 0.0f) noexcept
  {
    auto option = transition_timeline_->apply(&field_color_, color, duration);
    option.updateFn([this]() noexcept
                    {
                      field_shader_->uniform("u_color", field_color_());
                      bg_shader_->uniform("u_color", field_color_());
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
      index
    };

    field_panels_.push_back(panel);
  }

  // Blank更新
  void updateBlank(const std::vector<glm::ivec2>& blanks) noexcept
  {
    // 新しいのを追加
    for (const auto& pos : blanks)
    {
      if (searchBlank(pos)) continue; 

      glm::vec3 blank_pos { pos.x * PANEL_SIZE, 0, pos.y * PANEL_SIZE };
      blank_panels_.push_back({ pos, blank_pos });

      // Blank Panel出現演出
      auto& panel = blank_panels_.back();
      auto panel_pos = panel.position + blank_appear_pos_;
      timeline_->applyPtr(&panel.position,
                          panel_pos, panel.position,
                          blank_appear_duration_, getEaseFunc(blank_appear_ease_));

      panel.position = panel_pos;
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
    if (!p) {
      DOUT << "No blank position:" << pos << std::endl;
      return;
    }

    timeline_->removeTarget(&p->y);
    timeline_->applyPtr(&p->y, 0.0f, blank_touch_begin_pos_, blank_touch_begin_duration_, getEaseFunc(blank_touch_begin_ease_));
  }

  // Blankのタッチをやめた時の演出
  void blankTouchEndEase(const glm::vec2& pos) noexcept
  {
    auto* p = searchBlankPosition(pos);
    if (!p) {
      DOUT << "No blank position:" << pos << std::endl;
      return;
    }

    timeline_->removeTarget(&p->y);
    timeline_->applyPtr(&p->y, blank_touch_end_pos1_, blank_touch_end_duration1_, getEaseFunc(blank_touch_end_ease1_));
    timeline_->appendToPtr(&p->y, 0.0f, blank_touch_end_duration2_, getEaseFunc(blank_touch_end_ease2_));
  }

  void blankTouchCancelEase(const glm::vec2& pos) noexcept
  {
    auto* p = searchBlankPosition(pos);
    if (!p) {
      DOUT << "No blank position:" << pos << std::endl;
      return;
    }

    timeline_->removeTarget(&p->y);
    timeline_->applyPtr(&p->y, 0.0f, blank_touch_cancel_duration_, getEaseFunc(blank_touch_cancel_ease_));
  }

  // 得点した時の演出
  void startEffect(const glm::ivec2& pos) noexcept
  {
    glm::vec3 gpos{ pos.x * PANEL_SIZE, 0, pos.y * PANEL_SIZE };

    for (int i = 0; i < 10; ++i)
    {
      glm::vec3 ofs{
        ci::randFloat(-PANEL_SIZE / 2, PANEL_SIZE / 2),
        0,
        ci::randFloat(-PANEL_SIZE / 2, PANEL_SIZE / 2)
      };

      // ランダムに落下する立方体
      effects_.push_back({ true, gpos + ofs, glm::vec3() });
      auto& effect = effects_.back();

      auto end_pos = gpos + ofs + glm::vec3(0, ci::randFloat(15.0f, 30.0f), 0);

      float duration = ci::randFloat(1.25f, 1.75f);
      auto options = timeline_->applyPtr(&effect.pos, end_pos, duration);
      options.finishFn([&effect]() noexcept
                       {
                         effect.active = false;
                       });
    }
  }


  // ゲーム終了
  void endPlay() noexcept
  {
    for (auto& panel : blank_panels_)
    {
      timeline_->removeTarget(&panel.position);

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

#endif


private:
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

  // OBJ形式を読み込む
  static ci::gl::VboMeshRef loadObj(const std::string& path) noexcept
  {
    // 面法線は無しで
    ci::ObjLoader loader(Asset::load(path), false);
    auto mesh = ci::gl::VboMesh::create(ci::TriMesh(loader));

    return mesh;
  }

  // 影レンダリング用の設定
  void setupShadowMap(const glm::ivec2& fbo_size) noexcept
  {
    ci::gl::Texture2d::Format depthFormat;

    depthFormat.internalFormat(GL_DEPTH_COMPONENT16)
               .compareMode(GL_COMPARE_REF_TO_TEXTURE)
               .magFilter(GL_LINEAR)
               .minFilter(GL_LINEAR)
               .wrap(GL_CLAMP_TO_EDGE)	
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

  glm::vec3* searchBlankPosition(const glm::ivec2& pos) noexcept
  {
    for (auto& b : blank_panels_)
    {
      if (b.field_pos == pos) return &b.position;
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
    ci::gl::ScopedViewport viewport(glm::vec2(0), shadow_fbo_->getSize());
    ci::gl::clear(GL_DEPTH_BUFFER_BIT);
    ci::gl::setMatrices(light_camera_);

    ci::gl::ScopedGlslProg prog(shadow_shader_);

    drawFieldPanels();
    drawFieldBlank();

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

    ci::gl::ScopedGlslProg prog(field_shader_);
    ci::gl::ScopedTextureBind texScope(shadow_map_);

    drawFieldPanels();
    drawFieldBlank();

    if (info.playing)
    {
      // 手持ちパネル
      auto pos = panel_disp_pos_() + glm::vec3(0, height_offset_, 0);
      drawPanel(info.panel_index, pos, info.panel_rotation, rotate_offset_);

      // 選択箇所
      float s = std::abs(std::sin(put_gauge_timer_ * 6.0f)) * 0.1;
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
#if defined (DEBUG)
    if (disp_cloud_) drawClouds();
#endif
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
  void drawFieldPanels() noexcept
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

      const auto& model = getPanelModel(p.index);
      ci::gl::draw(model);
    }
  }
  
  // Fieldの置ける場所をすべて表示
  void drawFieldBlank() noexcept
  {
    ci::gl::ScopedModelMatrix m;

    for (const auto& p : blank_panels_)
    {
      auto mtx = glm::translate(p.position);
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
    ci::gl::ScopedModelMatrix m;

    for (auto it = std::begin(effects_); it != std::end(effects_); )
    {
      if (!it->active)
      {
        it = effects_.erase(it);
        continue;
      }

      auto mtx = glm::translate(it->pos) * glm::eulerAngleXYZ(it->rot.x, it->rot.y, it->rot.z);
      ci::gl::setModelMatrix(mtx);
      ci::gl::draw(effect_model);

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
  glm::vec3 bg_scale_;

  // Field上のパネル(Modelと被っている)
  struct Panel
  {
    glm::ivec2 field_pos;
    glm::vec3 position;
    glm::vec3 rotation;
    int index;
  };
  // NOTICE 追加時にメモリ上で再配置されるのを避けるためstd::vectorではない
  std::deque<Panel> field_panels_;

  struct Blank
  {
    glm::ivec2 field_pos;
    glm::vec3 position;
  };
  std::list<Blank> blank_panels_;

  // 得点時演出用
  ci::gl::VboMeshRef effect_model;

  ci::gl::GlslProgRef field_shader_;
  ci::Anim<ci::ColorA> field_color_ = ci::ColorA::white();

  ci::gl::GlslProgRef bg_shader_;
  ci::gl::Texture2dRef bg_texture_;

  ci::gl::GlslProgRef shadow_shader_;

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
  float put_gauge_timer_ = 0.0f;

  // FIXME 途中の削除が多いのでvectorは向いていない??
  std::list<Effect> effects_;


  // 雲演出
  std::vector<ci::gl::VboMeshRef> cloud_models_;
  ci::gl::Texture2dRef cloud_texture_;
  ci::gl::GlslProgRef cloud_shader_;
  std::vector<std::pair<glm::vec3, glm::vec3>> clouds_;
  glm::vec3 cloud_scale_;
  float cloud_area_;


  // Tween用
  ci::TimelineRef timeline_;
  // Pause中でも動作
  ci::TimelineRef force_timeline_;
  // リセットされたくない
  ci::TimelineRef transition_timeline_;


#if defined (DEBUG)
  bool disp_cloud_ = false;
#endif
};


#if defined (DEBUG)

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

