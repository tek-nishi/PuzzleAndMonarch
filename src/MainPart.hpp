#pragma once

//
// アプリ本編
//

#include "Defines.hpp"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cinder/Rand.h>
#include <cinder/Ray.h>
#include <cinder/Sphere.h>
#include "Params.hpp"
#include "JsonUtil.hpp"
#include "Game.hpp"
#include "Counter.hpp"
#include "View.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "ConnectionHolder.hpp"
#include "Task.hpp"
#include "CountExec.hpp"


namespace ngs {

class MainPart
  : public Task
{

public:
  MainPart(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : params_(params),
      event_(event),
      panels_(createPanels()),
      game_(std::make_unique<Game>(params["game"], event, panels_)),
      rotation(toRadians(Json::getVec<glm::vec2>(params["field.camera.rotation"]))),
      distance(params.getValueForKey<float>("field.camera.distance")),
      camera_(params["field.camera"]),
      pivot_point(Json::getVec<glm::vec3>(params["field.pivot_point"])),
      field_center_(pivot_point),
      panel_height_(params.getValueForKey<float>("field.panel_height")),
      putdown_time_(params.getValueForKey<double>("field.putdown_time")),
      view_(createView()),
      timeline_(ci::Timeline::create())
  {
    // フィールドカメラ
    auto& camera = camera_.body();
    glm::quat q(glm::vec3(rotation.x, rotation.y, 0));
    glm::vec3 p = q * glm::vec3{ 0, 0, -distance };
    camera.lookAt(p, glm::vec3(0));
    eye_position = camera.getEyePoint();

    // system
    holder_ += event_.connect("resize",
                              std::bind(&MainPart::resize,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("draw",
                              std::bind(&MainPart::draw,
                                        this, std::placeholders::_1, std::placeholders::_2));

    // 操作
    holder_ += event_.connect("single_touch_began",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (paused_) return;

                                draged_length_ = 0.0f;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));

                                // 元々パネルのある位置をタップ→長押しで設置
                                touch_put_ = false;
                                if (can_put_ && isCursorPos(touch.pos))
                                {
                                  touch_put_ = true;
                                  put_remaining_ = putdown_time_;
                                }
                              });

    
    holder_ += event_.connect("single_touch_moved",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (paused_) return;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                // ドラッグの距離を調べて、タップかドラッグか判定
                                draged_length_ += glm::distance(touch.pos, touch.prev_pos);
                                if (draged_length_ > 5.0f)
                                {
                                  touch_put_ = false;
                                }

                                // Field回転操作
                                rotateField(touch);

                                auto& camera = camera_.body();
                                glm::quat q(glm::vec3{ rotation.x, rotation.y, 0 });
                                glm::vec3 p = q * glm::vec3{ 0, 0, -distance };
                                camera.lookAt(p, glm::vec3(0));
                                eye_position = camera.getEyePoint();
                              });

    holder_ += event_.connect("single_touch_ended",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (paused_) return;

                                // 画面回転操作をした場合、パネル操作は全て無効
                                if (draged_length_ > 5.0f)
                                {
                                  DOUT << "draged_length: " << draged_length_ << std::endl;
                                  return;
                                }

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                // タッチ位置→升目位置
                                auto result = calcGridPos(touch.pos);
                                if (!result.first) return;

                                const auto& grid_pos = result.second;
                                if (grid_pos != field_pos_)
                                {
                                  // 可能であればパネルを移動
                                  calcNewFieldPos(grid_pos);
                                }
                                else
                                {
                                  // パネルを回転
                                  game_->rotationHandPanel();
                                  rotate_offset = 90.0f;
                                  can_put_ = game_->canPutToBlank(field_pos_);
                                  touch_put_ = false;
                                }
                              });

    holder_ += event_.connect("multi_touch_moved",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& touches = boost::any_cast<const std::vector<Touch>&>(arg.at("touches"));
                                auto& camera = camera_.body();

                                float l      = glm::distance(touches[0].pos, touches[1].pos);
                                float prev_l = glm::distance(touches[0].prev_pos, touches[1].prev_pos);
                                float dl = l - prev_l;
                                if (std::abs(dl) > 1.0f)
                                {
                                  // ピンチング
                                  distance = std::max(distance - dl * 0.25f, camera.getNearClip() + 1.0f);

                                  glm::quat q(glm::vec3(rotation.x, rotation.y, 0));
                                  glm::vec3 p = q * glm::vec3{ 0, 0, -distance };
                                  camera.lookAt(p, glm::vec3(0));
                                  eye_position = camera.getEyePoint();
                                }
                                else
                                {
                                  // 平行移動
                                  auto v = touches[0].pos - touches[0].prev_pos;

                                  glm::quat q(glm::vec3(0, rotation.y, 0));
                                  glm::vec3 p = q * glm::vec3(v.x, 0, v.y);
                                  eye_position += p;
                                  camera.setEyePoint(eye_position);
                                }
                              });

    // 各種イベント
    holder_ += event_.connect("Title:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                hight_offset = 500.0f;
                              });

    holder_ += event_.connect("Game:Start",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                game_->beginPlay();
                                calcNextPanelPosition();
                              });

    // holder_ += event_.connect("Game:Finish",
    //                           [this](const Connection&, const Arguments&) noexcept
    //                           {
    //                           });

    holder_ += event_.connect("pause:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                paused_ = true;
                              });
    
    holder_ += event_.connect("resume:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                paused_ = false;
                              });

#if defined (DEBUG)
    holder_ += event_.connect("debug-info",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                disp_debug_info_ = !disp_debug_info_;
                              });
#endif
  }


private:
  void resize(const Connection&, const Arguments&) noexcept
  {
    camera_.resize();
  }

	bool update(const double current_time, const double delta_time) noexcept override
  {
    if (paused_)
    {
      return true;
    }

    game_->update(delta_time);

    // カメラの中心位置変更
    pivot_point    += (field_center_ - pivot_point) * 0.05f;
    pivot_distance += (field_distance_ - pivot_distance) * 0.05f;

    if (game_->isPlaying())
    {
      // パネル設置操作
      if (touch_put_)
      {
        // タッチしたまま時間経過
        put_remaining_ -= delta_time;
        if (put_remaining_ <= 0.0)
        {
          game_->putHandPanel(field_pos_);

          // 次のパネルの準備
          rotate_offset = 0.0f;
          hight_offset  = 500.0f;
          can_put_      = false;

          touch_put_ = false;
          // 次のパネルを操作できないようにしとく
          draged_length_ = 100.0f;
            
          // Fieldの中心を再計算
          calcFieldCenter();
          calcNextPanelPosition();
        }
      }
    }

    frame_counter += 1;

    timeline_->step(delta_time);

    return true;
  }

  void draw(const Connection&, const Arguments&) noexcept
  {
    // 本編
    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);
    ci::gl::disableAlphaBlending();
    
    ci::gl::setMatrices(camera_.body());

    {
      glm::quat q(glm::vec3(rotation.x, rotation.y, 0));
      glm::vec3 p = q * glm::vec3{ 0, 0, pivot_distance };
      ci::gl::translate(p);
    }

    ci::gl::translate(-pivot_point);

    // フィールド
    const auto& field_panels = game_->getFieldPanels();
    drawFieldPanels(field_panels, view_);

    if (game_->isPlaying())
    {
      // 置ける場所
      const auto& blank = game_->getBlankPositions();
      ngs::drawFieldBlank(blank, view_);
      
      // 手持ちパネル
      rotate_offset *= 0.8f;
      hight_offset  *= 0.8f;
      glm::vec3 pos(cursor_pos_.x, cursor_pos_.y + hight_offset, cursor_pos_.z);
      ngs::drawPanel(game_->getHandPanel(), pos, game_->getHandRotation(), view_, rotate_offset);
      
      if (can_put_)
      {
        float s = std::abs(std::sin(frame_counter * 0.1)) * 0.1;
        glm::vec3 scale(0.9 + s, 1, 0.9 + s);
        drawFieldSelected(field_pos_, scale, view_);
        
        scale.x = 1.0 + s;
        scale.z = 1.0 + s;
        drawCursor(pos, scale, view_);
      }

#ifdef DEBUG
      if (disp_debug_info_)
      {
        // 手元のパネル
        ngs::drawPanelEdge(panels_[game_->getHandPanel()], pos, game_->getHandRotation());

        // 置こうとしている場所の周囲
        auto around = game_->enumerateAroundPanels(field_pos_);
        if (!around.empty()) {
          for (auto it : around) {
            auto p = it.first * int(ngs::PANEL_SIZE);
            glm::vec3 disp_pos(p.x, 0.0f, p.y);

            auto status = it.second;
            ngs::drawPanelEdge(panels_[status.number], disp_pos, status.rotation);
          }
        }
      }
#endif
    }
    drawFieldBg(view_);


#if 0
#if defined(DEBUG)
    // 外接球の表示
    ci::gl::enableDepth(false);
    ci::gl::enableAlphaBlending();

    ci::gl::pushModelView();
    ci::gl::color(0, 0.8, 0, 0.2);
    ci::gl::drawSphere(framing_sphere_);
    ci::gl::popModelView();
#endif
#endif
  }
  

  // タッチ位置からField上の升目座標を計算する
  std::pair<bool, glm::ivec2> calcGridPos(const glm::vec2& pos) const noexcept
  {
    // 画面奥に伸びるRayを生成
    ci::Ray ray = camera_.body().generateRay(pos, ci::app::getWindowSize());
    
    glm::quat q(glm::vec3(rotation.x, rotation.y, 0));
    glm::vec3 p = q * glm::vec3{ 0, 0, pivot_distance };
    auto m = glm::translate(p);
    m = glm::translate(m, -pivot_point);
    auto tpm = glm::inverse(m);

    auto origin = tpm * glm::vec4(ray.getOrigin(), 1);
    ray.setOrigin(origin);

    float z;
    float on_field = ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z);
    if (!on_field)
    {
      // なんらかの事情で位置を計算できず
      return { false, glm::ivec2() }; 
    }
    // ワールド座標→升目座標
    auto touch_pos = ray.calcPosition(z);
    return { true, roundValue(touch_pos.x, touch_pos.z, PANEL_SIZE) };
  }

  // タッチ位置がパネルのある位置と同じか調べる
  bool isCursorPos(const glm::vec2& pos) noexcept
  {
    auto result = calcGridPos(pos);
    if (!result.first) return false;

    const auto& grid_pos = result.second;
    return grid_pos == field_pos_;
  }

  // 升目位置からPanel位置を計算する
  void calcNewFieldPos(const glm::ivec2& grid_pos) noexcept
  {
    if (game_->isBlank(grid_pos))
    {
      field_pos_ = grid_pos;
      can_put_   = game_->canPutToBlank(field_pos_);
      // 少し宙に浮いた状態
      cursor_pos_ = glm::vec3(field_pos_.x * PANEL_SIZE, panel_height_, field_pos_.y * PANEL_SIZE);
    }
  }


  // Fieldの外接球を計算
  ci::Sphere calcBoundingSphere() noexcept
  {
    std::vector<glm::vec3> points;
#if 0
    {
      const auto& panels = game_->getFieldPanels();
      for (const auto& p : panels) {
        // Panelの４隅の座標を加える
        auto pos = p.position * int(PANEL_SIZE);
        points.push_back({ pos.x - PANEL_SIZE / 2, 0, pos.y - PANEL_SIZE / 2 });
        points.push_back({ pos.x + PANEL_SIZE / 2, 0, pos.y - PANEL_SIZE / 2 });
        points.push_back({ pos.x - PANEL_SIZE / 2, 0, pos.y + PANEL_SIZE / 2 });
        points.push_back({ pos.x + PANEL_SIZE / 2, 0, pos.y + PANEL_SIZE / 2 });
      }
    }
#endif

    {
      const auto& positions = game_->getBlankPositions();
      for (const auto& p : positions) {
        // Panelの４隅の座標を加える
        auto pos = p * int(PANEL_SIZE);
        points.push_back({ pos.x - PANEL_SIZE / 2, 0, pos.y - PANEL_SIZE / 2 });
        points.push_back({ pos.x + PANEL_SIZE / 2, 0, pos.y - PANEL_SIZE / 2 });
        points.push_back({ pos.x - PANEL_SIZE / 2, 0, pos.y + PANEL_SIZE / 2 });
        points.push_back({ pos.x + PANEL_SIZE / 2, 0, pos.y + PANEL_SIZE / 2 });
      }
    }

    auto sphere = ci::Sphere::calculateBoundingSphere(points);
    return sphere;
  }

  void calcFieldCenter() noexcept
  {
    auto sphere = calcBoundingSphere();
#if defined (DEBUG)
    framing_sphere_ = sphere;
#endif
    auto center = sphere.getCenter();
    field_center_.x = center.x;
    field_center_.z = center.z;

    // Sphereがちょうどすっぽり収まる距離を計算
    float radius = sphere.getRadius();
    const auto& camera = camera_.body();
    float fov_v = camera.getFov();
    float fov_h = camera.getFovHorizontal();
    float fov = (fov_v < fov_h) ? fov_v : fov_h;
    field_distance_ = (radius * 0.3f) / std::tan(ci::toRadians(fov * 0.5f));
    DOUT << "field distance: " << field_distance_ << std::endl;
  }


  // Touch座標→Field上の座標
  glm::vec3 calcTouchPos(const glm::vec2& touch_pos) noexcept
  {
    const auto& camera = camera_.body();
    ci::Ray ray = camera.generateRay(touch_pos, ci::app::getWindowSize());

    // auto m = glm::translate(pivot_point);
    // auto origin = m * glm::vec4(ray.getOrigin(), 1);
    // ray.setOrigin(origin);

    // 地面との交差を調べ、正確な位置を計算
    float z;
    if (!ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z))
    {
      // FIXME FieldとRayが交差しなかったら原点を返す
      return glm::vec3(0);
    }
    return ray.calcPosition(z);
  }

  // Touch座標からFieldの回転を計算
  void rotateField(const Touch& touch) noexcept
  {
    auto pos      = calcTouchPos(touch.pos);
    auto prev_pos = calcTouchPos(touch.prev_pos);

    // pos      -= pivot_point;
    // prev_pos -= pivot_point;

    // 正規化
    pos      = glm::normalize(pos);
    prev_pos = glm::normalize(prev_pos);

    // 外積から回転量が決まる
    float cross = prev_pos.x * pos.z - prev_pos.z * pos.x;
    rotation.y += std::asin(cross);
  }

  // 次のパネルの出現位置を決める
  void calcNextPanelPosition() noexcept
  {
    const auto& positions = game_->getBlankPositions();
    // FIXME とりあえず無作為に決める
    field_pos_  = positions[ci::randInt(int(positions.size()))];
    cursor_pos_ = glm::vec3(field_pos_.x * PANEL_SIZE, panel_height_, field_pos_.y * PANEL_SIZE);

    can_put_ = game_->canPutToBlank(field_pos_);
  }


  // FIXME 変数を後半に定義する実験
  const ci::JsonTree& params_;

  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;

  bool paused_ = false;

  std::vector<Panel> panels_;
  std::unique_ptr<Game> game_;

  // パネル操作
  float draged_length_;
  bool touch_put_;
  double put_remaining_;

  float panel_height_;
  double putdown_time_;

  // 実際のパネル位置
  glm::vec3 cursor_pos_;

  // パネルを配置しようとしている位置
  glm::ivec2 field_pos_;
  // 配置可能
  bool can_put_ = false;

  // 手持ちパネル演出用
  float rotate_offset = 0.0f;
  float hight_offset  = 0.0f;

  // アプリ起動から画面を更新した回数
  u_int frame_counter = 0;

  // カメラ関連
  glm::vec2 rotation;
  float distance;
  glm::vec3 eye_position;

  glm::vec3 pivot_point;
  float pivot_distance = 0.0f;

  // Fieldの中心座標
  glm::vec3 field_center_;
  float field_distance_ = 0.0f;

  // 表示
  Camera camera_;
  View view_;
  
  ci::TimelineRef timeline_;


#ifdef DEBUG
  bool disp_debug_info_ = false;

  // Fieldの外接球
  ci::Sphere framing_sphere_;
#endif
};

}
