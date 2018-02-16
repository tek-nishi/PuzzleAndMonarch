#pragma once

//
// アプリ本編
//

#include "Defines.hpp"
#include <tuple>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cinder/Rand.h>
#include <cinder/Ray.h>
#include <cinder/Sphere.h>
#include <cinder/CinderMath.h>
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
#include "EaseFunc.hpp"


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
      disp_ease_duration_(params.getValueForKey<float>("field.disp_ease_duration")),
      disp_ease_name_(params.getValueForKey<std::string>("field.disp_ease_name")),
      rotate_ease_duration_(params.getValueForKey<float>("field.rotate_ease_duration")),
      rotate_ease_name_(params.getValueForKey<std::string>("field.rotate_ease_name")),
      height_ease_start_(params.getValueForKey<float>("field.height_ease_start")),
      height_ease_duration_(params.getValueForKey<float>("field.height_ease_duration")),
      height_ease_name_(params.getValueForKey<std::string>("field.height_ease_name")),
      camera_rotation_(toRadians(Json::getVec<glm::vec2>(params["field.camera.rotation"]))),
      camera_distance_(params.getValueForKey<float>("field.camera.distance")),
      camera_distance_range_(Json::getVec<glm::vec2>(params["field.camera_distance_range"])),
      camera_(params["field.camera"]),
      target_position_(Json::getVec<glm::vec3>(params["field.target_position"])),
      field_center_(target_position_),
      panel_height_(params.getValueForKey<float>("field.panel_height")),
      putdown_time_(params.getValueForKey<double>("field.putdown_time")),
      view_(params["field"]),
      timeline_(ci::Timeline::create())
  {
    // フィールドカメラ
    calcCamera(camera_.body());
    field_distance_ = camera_distance_;

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

                                  auto ndc_pos = camera_.body().worldToNdc(cursor_pos_);
                                  Arguments args = {
                                    { "pos", ndc_pos }
                                  };
                                  event_.signal("Game:PutBegin", args);
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
                                if (touch_put_ && (draged_length_ > 5.0f))
                                {
                                  touch_put_ = false;
                                  event_.signal("Game:PutEnd", Arguments());
                                }

                                // Field回転操作
                                rotateField(touch);
                                calcCamera(camera_.body());
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

                                if (touch_put_)
                                {
                                  event_.signal("Game:PutEnd", Arguments());
                                }

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;
                                
                                if (isCursorPos(touch.pos))
                                {
                                  // パネルを回転
                                  game_->rotationHandPanel();
                                  startRotatePanelEase();
                                  can_put_ = game_->canPutToBlank(field_pos_);
                                  touch_put_ = false;
                                  return;
                                }

                                // タッチ位置→升目位置
                                auto result = calcGridPos(touch.pos);
                                if (!std::get<0>(result)) return;
                                const auto& grid_pos = std::get<1>(result);
                                // 可能であればパネルを移動
                                calcNewFieldPos(grid_pos);
                              });

    holder_ += event_.connect("multi_touch_moved",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& touches = boost::any_cast<const std::vector<Touch>&>(arg.at("touches"));
                                auto& camera = camera_.body();

                                // TIPS ２つのタッチ位置の移動ベクトル→内積
                                //      回転操作か平行移動操作か判別できる
                                auto d0  = touches[0].pos - touches[0].prev_pos;
                                auto d1  = touches[1].pos - touches[1].prev_pos;
                                auto dot = glm::dot(d0, d1);
                                
                                // ２つのタッチ位置の距離変化→ズーミング
                                float l0 = glm::distance(touches[0].pos, touches[1].pos);
                                float l1 = glm::distance(touches[0].prev_pos, touches[1].prev_pos);
                                auto dl = l0 - l1;
                                if ((std::abs(dl) > 1.0f) && (l0 != 0.0f) && (l1 != 0.0f))
                                {
                                  // ピンチング
                                  float n = l0 / l1;
                                  camera_distance_ = ci::clamp(camera_distance_ / n,
                                                               camera_distance_range_.x, camera_distance_range_.y);
                                  calcCamera(camera);
                                  field_distance_ = camera_distance_;
                                }
                                if (dot > 0.0f)
                                {
                                  // 平行移動
                                  // TIPS ２点をRay castでworld positionに変換して
                                  //      差分→移動量
                                  auto pos = (touches[0].pos + touches[1].pos) * 0.5f;
                                  auto ray1 = camera.generateRay(pos, ci::app::getWindowSize());
                                  float z1;
                                  ray1.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z1);
                                  auto p1 = ray1.calcPosition(z1);

                                  auto prev_pos = (touches[0].prev_pos + touches[1].prev_pos) * 0.5f;
                                  auto ray2 = camera.generateRay(prev_pos, ci::app::getWindowSize());
                                  float z2;
                                  ray2.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z2);
                                  auto p2 = ray2.calcPosition(z2);

                                  auto v = p2 - p1;
                                  v.y = 0;

                                  target_position_ += v;
                                  field_center_ = target_position_;
                                  eye_position_ += v;
                                  camera.setEyePoint(eye_position_);
                                }
                              });

    // 各種イベント
    // holder_ += event_.connect("Title:finished",
    //                           [this](const Connection&, const Arguments&) noexcept
    //                           {
    //                           });

    holder_ += event_.connect("Game:Start",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                game_->beginPlay();
                                calcNextPanelPosition();
                                startNextPanelEase();
                              });

    holder_ += event_.connect("Game:PutPanel",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                auto panel      = boost::any_cast<int>(args.at("panel"));
                                const auto& pos = boost::any_cast<glm::ivec2>(args.at("field_pos"));
                                auto rotation   = boost::any_cast<u_int>(args.at("rotation"));
                                view_.addPanel(panel, pos, rotation);
                                view_.startPutEase(timeline_);
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
                              [this](const Connection&, Arguments&) noexcept
                              {
                                disp_debug_info_ = !disp_debug_info_;
                              });
    
    holder_ += event_.connect("debug-main-draw",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                debug_draw_ = !debug_draw_;
                              });
#endif
    // 本編準備
    game_->preparationPlay();
  }


private:
  void resize(const Connection&, const Arguments&) noexcept
  {
    camera_.resize();
  }

	bool update(double current_time, double delta_time) noexcept override
  {
    if (paused_)
    {
      return true;
    }

    game_->update(delta_time);

    // カメラの中心位置変更
    target_position_ += (field_center_ - target_position_) * 0.025f;
    camera_distance_ += (field_distance_ - camera_distance_) * 0.05f;
    calcCamera(camera_.body());

    if (game_->isPlaying())
    {
      // パネル設置操作
      if (touch_put_)
      {
        // タッチしたまま時間経過
        put_remaining_ -= delta_time;

        {
          auto ndc_pos = camera_.body().worldToNdc(cursor_pos_);
          auto scale = 1.0f - glm::clamp(float(put_remaining_ / putdown_time_), 0.0f, 1.0f);
          Arguments args = {
            { "pos",   ndc_pos },
            { "scale", scale },
          };
          event_.signal("Game:PutHold", args);
        }

        if (put_remaining_ <= 0.0)
        {
          game_->putHandPanel(field_pos_);
          event_.signal("Game:PutEnd", Arguments());

          // 次のパネルの準備
          rotate_offset_.stop();
          rotate_offset_ = 0.0f;
          can_put_       = false;

          startNextPanelEase();

          touch_put_ = false;
          // 次のパネルを操作できないようにしとく
          draged_length_ = 100.0f;
            
          // Fieldの中心を再計算
          calcViewRange();
          calcNextPanelPosition();
        }
      }
    }

    put_gauge_timer_ += delta_time;

    timeline_->step(delta_time);

    return true;
  }

  void draw(const Connection&, const Arguments&) noexcept
  {
#if defined (DEBUG)
    if (debug_draw_) return;
#endif

    // 本編
    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);
    ci::gl::disableAlphaBlending();
    ci::gl::setMatrices(camera_.body());

    // フィールド
    view_.drawFieldPanels();

    if (game_->isPlaying())
    {
      // 置ける場所
      const auto& blank = game_->getBlankPositions();
      view_.drawFieldBlank(blank);

      // 手持ちパネル
      glm::vec3 pos(panel_disp_pos_().x, panel_disp_pos_().y + height_offset_, panel_disp_pos_().z);
      view_.drawPanel(game_->getHandPanel(), pos, game_->getHandRotation(), rotate_offset_);

      float s = std::abs(std::sin(put_gauge_timer_ * 6.0f)) * 0.1;
      glm::vec3 scale(0.9 + s, 1, 0.9 + s);
      view_.drawFieldSelected(field_pos_, scale);
      
      if (can_put_)
      {
        scale.x = 1.0 + s;
        scale.z = 1.0 + s;
        view_.drawCursor(pos, scale);
      }

#ifdef DEBUG
      if (disp_debug_info_)
      {
        if (game_->isPlaying())
        {
          // 手元のパネル
          // drawPanelEdge(panels_[game_->getHandPanel()], pos, game_->getHandRotation());
          
          // 置こうとしている場所の周囲
          auto around = game_->enumerateAroundPanels(field_pos_);
          if (!around.empty())
          {
            for (auto it : around)
            {
              auto p = it.first * int(PANEL_SIZE);
              glm::vec3 disp_pos(p.x, 0.0f, p.y);

              auto status = it.second;
              drawPanelEdge(panels_[status.number], disp_pos, status.rotation);
            }
          }

          // パネルのAABB
          auto aabb = view_.panelAabb(game_->getHandPanel());
          aabb.transform(glm::translate(cursor_pos_));
          ci::gl::color(0, 1, 0);
          ci::gl::drawStrokedCube(aabb);
        }
      }
#endif
    }
    view_.drawFieldBg();


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
  
  void calcCamera(ci::CameraPersp& camera) noexcept
  {
    glm::quat q(glm::vec3{ camera_rotation_.x, camera_rotation_.y, 0 });
    glm::vec3 p = q * glm::vec3{ 0, 0, -camera_distance_ };
    camera.lookAt(p + target_position_, target_position_);
    eye_position_ = camera.getEyePoint();
  }


  // タッチ位置からField上の升目座標を計算する
  std::tuple<bool, glm::ivec2, ci::Ray> calcGridPos(const glm::vec2& pos) const noexcept
  {
    // 画面奥に伸びるRayを生成
    ci::Ray ray = camera_.body().generateRay(pos, ci::app::getWindowSize());

    float z;
    float on_field = ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z);
    if (!on_field)
    {
      // なんらかの事情で位置を計算できず
      return { false, glm::ivec2(), ray }; 
    }
    // ワールド座標→升目座標
    auto touch_pos = ray.calcPosition(z);
    return { true, roundValue(touch_pos.x, touch_pos.z, PANEL_SIZE), ray };
  }

  // タッチ位置がパネルのある位置と同じか調べる
  bool isCursorPos(const glm::vec2& pos) const noexcept
  {
    auto result = calcGridPos(pos);
    if (!std::get<0>(result)) return false;

    // パネルとのRay-cast
    auto aabb = view_.panelAabb(game_->getHandPanel());
    const auto& ray = std::get<2>(result);
    aabb.transform(glm::translate(cursor_pos_));
    if (aabb.intersects(ray)) return true;

    // タッチ位置の座標が一致しているかで判断
    const auto& grid_pos = std::get<1>(result);
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
      startMoveEase();
    }
  }


  // Fieldの外接球を計算
  ci::Sphere calcBoundingSphere() const noexcept
  {
    std::vector<glm::vec3> points;
    const auto& positions = game_->getBlankPositions();
    for (const auto& p : positions)
    {
      // Panelの４隅の座標を加える
      auto pos = p * int(PANEL_SIZE);
      points.emplace_back(pos.x - PANEL_SIZE / 2, 0.0f, pos.y - PANEL_SIZE / 2);
      points.emplace_back(pos.x + PANEL_SIZE / 2, 0.0f, pos.y - PANEL_SIZE / 2);
      points.emplace_back(pos.x - PANEL_SIZE / 2, 0.0f, pos.y + PANEL_SIZE / 2);
      points.emplace_back(pos.x + PANEL_SIZE / 2, 0.0f, pos.y + PANEL_SIZE / 2);
    }

    return ci::Sphere::calculateBoundingSphere(points);
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
    field_distance_ = std::max(radius / std::sin(ci::toRadians(fov * 0.5f)),
                               camera_distance_);

    DOUT << "field distance: " << field_distance_ << std::endl;
  }

  // フィールドの広さから注視点と距離を計算
  void calcViewRange() noexcept
  {
    std::vector<glm::vec3> points;
    const auto& positions = game_->getBlankPositions();
    for (const auto& p : positions)
    {
      // Panelの４隅の座標を加える
      auto pos = p * int(PANEL_SIZE);
      points.emplace_back(pos.x - PANEL_SIZE / 2, 0.0f, pos.y - PANEL_SIZE / 2);
      points.emplace_back(pos.x + PANEL_SIZE / 2, 0.0f, pos.y - PANEL_SIZE / 2);
      points.emplace_back(pos.x - PANEL_SIZE / 2, 0.0f, pos.y + PANEL_SIZE / 2);
      points.emplace_back(pos.x + PANEL_SIZE / 2, 0.0f, pos.y + PANEL_SIZE / 2);
    }

    // 登録した点の平均値→注視点
    auto center = std::accumulate(std::begin(points), std::end(points), glm::vec3()) / float(points.size());
    field_center_.x = center.x;
    field_center_.z = center.z;

    // 中心から一番遠い座標から距離を決める
    auto it = std::max_element(std::begin(points), std::end(points),
                               [center](const glm::vec3& a, const glm::vec3& b) noexcept
                               {
                                 auto d1 = glm::distance2(a, center);
                                 auto d2 = glm::distance2(b, center);
                                 return d1 < d2;
                               });

    auto d = glm::distance(*it, center);
    const auto& camera = camera_.body();
    float fov_v = camera.getFov();
    float fov_h = camera.getFovHorizontal();
    float fov = (fov_v < fov_h) ? fov_v : fov_h;
    float distance = d / std::tan(ci::toRadians(fov * 0.5f));

    float n = d / std::cos(camera_rotation_.x);
    distance -= n;
    field_distance_ = std::max(distance, camera_distance_);

    DOUT << "field distance: " << distance << "," << camera_distance_ << std::endl;
  }

  // Touch座標→Field上の座標
  glm::vec3 calcTouchPos(const glm::vec2& touch_pos) const noexcept
  {
    const auto& camera = camera_.body();
    ci::Ray ray = camera.generateRay(touch_pos, ci::app::getWindowSize());

    // 地面との交差を調べ、正確な位置を計算
    float z;
    if (!ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z))
    {
      // FIXME FieldとRayが交差しなかったら原点を返す
      return glm::vec3();
    }
    return ray.calcPosition(z);
  }

  // Touch座標からFieldの回転を計算
  void rotateField(const Touch& touch) noexcept
  {
    auto pos      = calcTouchPos(touch.pos);
    auto prev_pos = calcTouchPos(touch.prev_pos);

    pos      -= target_position_;
    prev_pos -= target_position_;

    // 正規化
    pos      = glm::normalize(pos);
    prev_pos = glm::normalize(prev_pos);

    // 外積から回転量が決まる
    float cross = prev_pos.x * pos.z - prev_pos.z * pos.x;
    camera_rotation_.y += std::asin(cross);
  }

  // 次のパネルの出現位置を決める
  void calcNextPanelPosition() noexcept
  {
    const auto& positions = game_->getBlankPositions();

    // 置いた場所から一番距離の近い場所を選ぶ
    auto it = std::min_element(std::begin(positions), std::end(positions),
                               [this](const glm::ivec2& a, const glm::ivec2& b) noexcept
                               {
                                 // FIXME 整数型のベクトルだとdistance2とかdotとかが使えない
                                 auto da = field_pos_ - a;
                                 auto db = field_pos_ - b;

                                 return (da.x * da.x + da.y * da.y) < (db.x * db.x + db.y * db.y);
                               });
    field_pos_ = *it;
    cursor_pos_ = glm::vec3(field_pos_.x * PANEL_SIZE, panel_height_, field_pos_.y * PANEL_SIZE);
    panel_disp_pos_.stop();
    panel_disp_pos_ = cursor_pos_;

    can_put_ = game_->canPutToBlank(field_pos_);
  }

  // パネル移動演出
  void startMoveEase() noexcept
  {
    panel_disp_pos_.stop();
    timeline_->apply(&panel_disp_pos_, cursor_pos_, disp_ease_duration_, getEaseFunc(disp_ease_name_));
  }

  // パネル回転演出
  void startRotatePanelEase() noexcept
  {
    rotate_offset_.stop();
    rotate_offset_ = 90.0f;
    timeline_->apply(&rotate_offset_, 0.0f, rotate_ease_duration_, getEaseFunc(rotate_ease_name_));
  }


  // 次のパネルの出現演出
  void startNextPanelEase() noexcept
  {
    height_offset_.stop();
    height_offset_ = height_ease_start_;
    timeline_->apply(&height_offset_, 0.0f, height_ease_duration_, getEaseFunc(height_ease_name_));
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

  // パネル位置
  ci::Anim<glm::vec3> panel_disp_pos_;
  float disp_ease_duration_;
  std::string disp_ease_name_;

  // パネルの回転
  ci::Anim<float> rotate_offset_ = 0.0f;
  float rotate_ease_duration_;
  std::string rotate_ease_name_;

  // 次のパネルを引いた時の演出
  ci::Anim<float> height_offset_ = 0.0f;
  float height_ease_start_;
  float height_ease_duration_;
  std::string height_ease_name_;

  // パネルを置くゲージ演出
  float put_gauge_timer_ = 0.0f;

  // カメラ関連
  glm::vec2 camera_rotation_;
  float camera_distance_;
  glm::vec3 target_position_;
  glm::vec3 eye_position_;

  // ピンチング操作時の距離の範囲
  glm::vec2 camera_distance_range_;

  // Fieldの中心座標
  glm::vec3 field_center_;
  float field_distance_ = 0.0f;

  // 表示
  Camera camera_;
  View view_;
  
  ci::TimelineRef timeline_;


#ifdef DEBUG
  bool disp_debug_info_ = false;
  bool debug_draw_ = false;

  // Fieldの外接球
  ci::Sphere framing_sphere_;
#endif
};

}
