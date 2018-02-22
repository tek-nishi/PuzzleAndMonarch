#pragma once

//
// テスト用タスク
//

#include "Task.hpp"
#include "Camera.hpp"


namespace ngs {

class DebugTask
  : public Task
{
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  Camera camera_;

  UI::Drawer& drawer_;

  bool active_ = true;
  bool disp_   = true;

  u_int disp_index_ = 0;

  
  
  bool update(double current_time, double delta_time) noexcept override
  {
    return active_;
  }

  void draw(const Connection&, const Arguments&) noexcept
  {
    if (!disp_) return;

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
      for(const auto& it : fonts)
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
    camera_.body().lookAt(glm::vec3(0, 0, camera_.getNearClip() + 0.001f), glm::vec3(0));

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
  }

  ~DebugTask() = default;
}; 

}
