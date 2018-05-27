#pragma once

//
// テスト用タスク
//

#if defined (DEBUG) && !defined (CINDER_COCOA_TOUCH)

#include <cinder/params/Params.h>
#include "Task.hpp"
#include "Camera.hpp"

// TIPS AntTweakBarを直接使う
extern "C" {
int TwDefine(const char *def);
}


namespace ngs {

class DebugTask
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;
  Camera camera_;

  UI::Drawer& drawer_;

  bool active_ = true;
  bool disp_   = false;

  u_int disp_index_ = 0;
  
  
  ci::params::InterfaceGlRef settings_;

  float font_buffer_ = 0.5f;
  float font_gamma_  = 0.03f;

  float polygon_factor_;
  float polygon_units_;

  glm::vec3 light_pos_;

  ci::ColorA field_specular_;
  float field_shininess_;
  float field_ambient_;

  ci::ColorA bg_specular_;
  float bg_shininess_;
  float bg_ambient_;


  void createSettings() noexcept
  {
    settings_ = ci::params::InterfaceGl::create("Settings", glm::ivec2(560, 800));

    settings_->addParam("Font:buffer", &font_buffer_)
    .precision(2)
    .step(0.01f)
    .updateFn([this]() noexcept
              {
                drawer_.setFontShaderParams({ font_buffer_, font_gamma_ });
              });

    settings_->addParam("Font:gamma", &font_gamma_)
    .precision(2)
    .step(0.01f)
    .updateFn([this]() noexcept
              {
                drawer_.setFontShaderParams({ font_buffer_, font_gamma_ });
              });

    settings_->addSeparator();

    // 影
    settings_->addParam("Shadow:factor", &polygon_factor_)
    .precision(2)
    .step(0.01f)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", polygon_factor_ }
                };
                event_.signal("debug-polygon-factor", args);
              });

    settings_->addParam("Shadow:units", &polygon_units_)
    .precision(2)
    .step(0.01f)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", polygon_units_ }
                };
                event_.signal("debug-polygon-units", args);
              });

    settings_->addSeparator();

    // ライティング
    settings_->addParam("Light:Position", &light_pos_)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", light_pos_ }
                };
                event_.signal("debug-light-position", args);
              });

    settings_->addSeparator();

    settings_->addParam("Field:Specular", &field_specular_)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", field_specular_ }
                };
                event_.signal("debug-field-specular", args);
              });

    settings_->addParam("Field:Shininess", &field_shininess_)
    .step(0.5f)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", field_shininess_ }
                };
                event_.signal("debug-field-shininess", args);
              });

    settings_->addParam("Field:Ambient", &field_ambient_)
    .step(0.01f)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", field_ambient_ }
                };
                event_.signal("debug-field-ambient", args);
              });
    
    settings_->addSeparator();

    settings_->addParam("Bg:Specular", &bg_specular_)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", bg_specular_ }
                };
                event_.signal("debug-bg-specular", args);
              });

    settings_->addParam("Bg:Shininess", &bg_shininess_)
    .step(0.5f)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", bg_shininess_ }
                };
                event_.signal("debug-bg-shininess", args);
              });

    settings_->addParam("Bg:Ambient", &bg_ambient_)
    .step(0.01f)
    .updateFn([this]() noexcept
              {
                Arguments args{
                  { "value", bg_ambient_ }
                };
                event_.signal("debug-bg-ambient", args);
              });

    settings_->show(false);

    // TIPS HELPを隠す
    TwDefine("TW_HELP visible=false");
  }

  void drawSettings() noexcept
  {
    settings_->draw();
  }
  

  bool update(double current_time, double delta_time) noexcept override
  {
    return active_;
  }


  void previewFont() noexcept
  {
    ci::gl::enableDepth(false);
    ci::gl::disable(GL_CULL_FACE);
    ci::gl::enableAlphaBlending(false);

    auto& camera = camera_.body();

    // TIPS CameraのNearClipでの四隅の座標を取得→描画領域
    glm::vec3 top_left;
    glm::vec3 top_right;
    glm::vec3 bottom_left;
    glm::vec3 bottom_right;
    camera.getNearClipCoordinates(&top_left, &top_right, &bottom_left, &bottom_right);
    ci::Rectf view_rect(top_left.x, bottom_right.y, bottom_right.x, top_left.y);

    {
      ci::gl::ScopedModelMatrix m;
      ci::gl::ScopedGlslProg prog(drawer_.getFontShader());
      
      ci::gl::setMatrices(camera);
      
      const auto& fonts = drawer_.getFonts();
      std::vector<const Font*> f;
      for (const auto& it : fonts)
      {
        f.push_back(&(it.second));
      }
      assert(f.size());

      auto index = disp_index_ % f.size();
      const auto& texture = f[index]->texture();

      auto size = std::min(view_rect.getWidth(), view_rect.getHeight()) * 0.9;
      ci::Rectf r(-size / 2, -size / 2, size / 2, size / 2);
      ci::gl::draw(texture, r);
    }
  }


  void draw(const Connection&, const Arguments&) noexcept
  {
    if (disp_)
    {
      previewFont();
    }

    drawSettings();
  }

  void resize(const Connection&, const Arguments&) noexcept
  {
    camera_.resize();
  }


public:
  DebugTask(const ci::JsonTree& params, Event<Arguments>& event, UI::Drawer& drawer) noexcept
    : event_(event),
      camera_(params["field.camera"]),
      drawer_(drawer)
  {
    // FIXME near_zピッタリの位置だとmacOSのReleaseビルドで絵が出ない
    camera_.body().lookAt(glm::vec3(0, 0, camera_.getNearClip() + 0.001f), glm::vec3());

    auto polygon_offset = Json::getVec<glm::vec2>(params["field.polygon_offset"]);
    polygon_factor_ = polygon_offset.x;
    polygon_units_  = polygon_offset.y;

    light_pos_ = Json::getVec<glm::vec3>(params["field.light.pos"]);

    field_specular_  = Json::getColorA<float>(params["field.field.specular"]);
    field_shininess_ = params.getValueForKey<float>("field.field.shininess");
    field_ambient_   = params.getValueForKey<float>("field.field.ambient");

    bg_specular_  = Json::getColorA<float>(params["field.bg.specular"]);
    bg_shininess_ = params.getValueForKey<float>("field.bg.shininess");
    bg_ambient_   = params.getValueForKey<float>("field.bg.ambient");

    holder_ += event_.connect("draw", 99,
                              std::bind(&DebugTask::draw,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("resize",
                              std::bind(&DebugTask::resize,
                                        this, std::placeholders::_1, std::placeholders::_2));


    holder_ += event_.connect("debug-font",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                disp_ = !disp_;
                              });

    holder_ += event_.connect("single_touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                ++disp_index_;
                              });

    holder_ += event_.connect("debug-settings",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                settings_->show(!settings_->isVisible());
                              });

    createSettings();
  }

  ~DebugTask() = default;
}; 

}

#endif
