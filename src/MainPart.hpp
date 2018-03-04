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
#include "FixedTimeExec.hpp"
#include "EaseFunc.hpp"
#include "Archive.hpp"
#include "Score.hpp"


namespace ngs {

class MainPart
  : public Task
{

public:
  MainPart(const ci::JsonTree& params, Event<Arguments>& event, Archive& archive) noexcept
    : params_(params),
      event_(event),
      archive_(archive),
      panels_(createPanels()),
      game_(std::make_unique<Game>(params["game"], event, panels_)),
      draged_max_length_(params.getValueForKey<float>("field.draged_max_length")),
      disp_ease_duration_(Json::getVec<glm::vec2>(params["field.disp_ease_duration"])),
      disp_ease_name_(params.getValueForKey<std::string>("field.disp_ease_name")),
      rotate_ease_duration_(Json::getVec<glm::vec2>(params["field.rotate_ease_duration"])),
      rotate_ease_name_(params.getValueForKey<std::string>("field.rotate_ease_name")),
      height_ease_start_(params.getValueForKey<float>("field.height_ease_start")),
      height_ease_duration_(params.getValueForKey<float>("field.height_ease_duration")),
      height_ease_name_(params.getValueForKey<std::string>("field.height_ease_name")),
      camera_rotation_(toRadians(Json::getVec<glm::vec2>(params["field.camera.rotation"]))),
      camera_distance_(params.getValueForKey<float>("field.camera.distance")),
      camera_distance_range_(Json::getVec<glm::vec2>(params["field.camera_distance_range"])),
      target_rate_(Json::getVec<glm::vec2>(params["field.target_rate"])),
      distance_rate_(Json::getVec<glm::vec2>(params["field.distance_rate"])),
      camera_(params["field.camera"]),
      target_position_(Json::getVec<glm::vec3>(params["field.target_position"])),
      field_center_(target_position_),
      panel_height_(params.getValueForKey<float>("field.panel_height")),
      putdown_time_(Json::getVec<glm::vec2>(params["field.putdown_time"])),
      view_(params["field"]),
      pause_duration_(Json::getVec<glm::vec2>(params["field.pause_duration"])),
      pause_ease_(params.getValueForKey<std::string>("field.pause_ease")),
      timeline_(ci::Timeline::create()),
      force_timeline_(ci::Timeline::create()),
      ranking_records_(params.getValueForKey<u_int>("game.ranking_records")),
      transition_duration_(params.getValueForKey<float>("ui.transition.duration")),
      transition_color_(Json::getColorA<float>(params["ui.transition.color"]))
  {
    // 乱数
    std::random_device seed_gen;
    engine_ = std::mt19937(seed_gen());

    // フィールドカメラ
    calcCamera(camera_.body());
    field_distance_ = camera_distance_;

    // system
    holder_ += event_.connect("resize",
                              std::bind(&MainPart::resize,
                                        this, std::placeholders::_1, std::placeholders::_2));
    
    holder_ += event_.connect("draw", 0,
                              std::bind(&MainPart::draw,
                                        this, std::placeholders::_1, std::placeholders::_2));

    // 操作
    holder_ += event_.connect("single_touch_began",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (!game_->isPlaying() || paused_ || prohibited_) return;

                                draged_length_ = 0.0f;
                                manipulated_   = false;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));

                                // 元々パネルのある位置をタップ→長押しで設置
                                touch_put_ = false;
                                if (can_put_ && isCursorPos(touch.pos))
                                {
                                  touch_put_ = true;
                                  // ゲーム終盤さっさとパネルを置ける
                                  current_putdown_time_ = glm::mix(putdown_time_.x, putdown_time_.y, game_->getPlayTimeRate());
                                  put_remaining_ = current_putdown_time_;

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
                                if (paused_ || prohibited_) return;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                // ドラッグの距離を調べて、タップかドラッグか判定
                                draged_length_ += glm::distance(touch.pos, touch.prev_pos);
                                if (touch_put_ && (draged_length_ > draged_max_length_))
                                {
                                  touch_put_ = false;
                                  event_.signal("Game:PutEnd", Arguments());
                                }

                                manipulated_ = draged_length_ > 5.0f;

                                // Field回転操作
                                rotateField(touch);
                                calcCamera(camera_.body());
                              });

    holder_ += event_.connect("single_touch_ended",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (!game_->isPlaying() || paused_ || prohibited_) return;

                                // 画面回転操作をした場合、パネル操作は全て無効
                                if (draged_length_ > draged_max_length_)
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
                                  can_put_   = game_->canPutToBlank(field_pos_);
                                  touch_put_ = false;
                                  game_event_.insert("Panel:rotate");
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
                                if (paused_ || prohibited_) return;

                                manipulated_ = true;
                                 
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
    holder_ += event_.connect("Title:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                game_event_.insert("Game:ready");
                              });

    holder_ += event_.connect("Game:Start",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                game_->beginPlay();
                                calcNextPanelPosition();
                                startNextPanelEase();
                              });

    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // 中断
                                archive_.addRecord("abort-times", uint32_t(1));
                                archive_.save();
                                resetGame();
                              });

    holder_ += event_.connect("Game:PutPanel",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                auto panel      = boost::any_cast<int>(args.at("panel"));
                                const auto& pos = boost::any_cast<glm::ivec2>(args.at("field_pos"));
                                auto rotation   = boost::any_cast<u_int>(args.at("rotation"));
                                view_.addPanel(panel, pos, rotation);
                                view_.startPutEase(timeline_, game_->getPlayTimeRate());
                              });

    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                DOUT << "Game:Finish" << std::endl;

                                force_camera_ = true;
                                prohibited_   = true;
                                manipulated_  = false;
                                game_event_.insert("Game:finish");

                                calcViewRange(false);

                                // スコア計算
                                auto score = calcGameScore(args);
                                recordGameScore(score);

                                count_exec_.add(2.3,
                                                [this, score]() noexcept
                                                {
                                                  Arguments a {
                                                    { "score", score } 
                                                  };
                                                  event_.signal("Result:begin", a);
                                                });

                                count_exec_.add(3.0,
                                                [this]() noexcept
                                                {
                                                  force_camera_ = false;
                                                  prohibited_   = false;
                                                });
                                fixed_exec_.add(1.5, -1.0,
                                                [this](double delta_time) noexcept
                                                {
                                                  camera_rotation_.y += M_PI * 0.025 * delta_time;
                                                  calcCamera(camera_.body());

                                                  return !manipulated_;
                                                });
                              });

    // 得点時の演出
    holder_ += event.connect("Game:completed_forests",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               auto completed = boost::any_cast<std::vector<std::vector<glm::ivec2>>>(args.at("completed"));
                               for (const auto& cc : completed)
                               {
                                 for (const auto& p : cc)
                                 {
                                   view_.startEffect(timeline_, p);
                                 }
                               }
                               game_event_.insert("Comp:forests");
                             });

    holder_ += event.connect("Game:completed_path",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               auto completed = boost::any_cast<std::vector<std::vector<glm::ivec2>>>(args.at("completed"));
                               for (const auto& cc : completed)
                               {
                                 for (const auto& p : cc)
                                 {
                                   view_.startEffect(timeline_, p);
                                 }
                               }
                               game_event_.insert("Comp:path");
                             });
    
    holder_ += event.connect("Game:completed_church",
                             [this](const Connection&, const Arguments& args) noexcept
                             {
                               auto completed = boost::any_cast<std::vector<glm::ivec2>>(args.at("completed"));
                               for (const auto& p : completed)
                               {
                                 view_.startEffect(timeline_, p);
                               }
                               game_event_.insert("Comp:church");
                             });

    // Result→Title
    holder_ += event.connect("Result:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                resetGame();
                              });

    // Ranking
    holder_ += event_.connect("Ranking:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                force_camera_ = true;

                                timeline_->clear();
                                view_.clear();

                                // TOPの記録を読み込む
                                {
                                  const auto& json = archive_.getRecordArray("games");
                                  if (json.hasChildren())
                                  {
                                    game_->load(json[0].getValueForKey("path"));
                                  }
                                }

                                calcViewRange(false);
                                
                                fixed_exec_.add(3.0, -1.0,
                                                [this](double delta_time) noexcept
                                                {
                                                  camera_rotation_.y += M_PI * 0.025 * delta_time;
                                                  calcCamera(camera_.body());

                                                  return !manipulated_;
                                                });
                              });
    
    holder_ += event_.connect("Ranking:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                force_camera_ = false;

                                timeline_->clear();
                                view_.clear();
                                // Game再生成
                                game_ = std::make_unique<Game>(params_["game"], event_, panels_);
                                game_->preparationPlay(engine_);

                                field_center_   = glm::vec3();
                                field_distance_ = params_.getValueForKey<float>("field.camera.distance");
                                fixed_exec_.clear();
                              });

    holder_ += event_.connect("App:ResignActive",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                if (game_->isPlaying() && !paused_)
                                {
                                  event_.signal("pause:touch_ended", Arguments());
                                }
                              });

    holder_ += event_.connect("pause:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // Pause開始
                                paused_ = true;
                                count_exec_.pause();
                                count_exec_.add(0.3,
                                                [this]() noexcept
                                                {
                                                  view_.setColor(force_timeline_, transition_duration_, transition_color_);
                                                  view_.setPauseEffect(toRadians(180.0f), force_timeline_,
                                                                       pause_duration_.x, pause_ease_);
                                                },
                                                true);
                              });

    holder_ += event_.connect("resume:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(0.3,
                                                [this]() noexcept
                                                {
                                                  view_.setColor(force_timeline_, transition_duration_, ci::ColorA(1, 1, 1, 1));
                                                  view_.setPauseEffect(0.0f, force_timeline_,
                                                                       pause_duration_.y, pause_ease_);
                                                },
                                                true);
                              });

    holder_ += event_.connect("abort:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(0.3,
                                                [this]() noexcept
                                                {
                                                  view_.setColor(force_timeline_, transition_duration_, ci::ColorA(1, 1, 1, 1));
                                                },
                                                true);
                              });
    
    holder_ += event_.connect("GameMain:resume",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                paused_ = false;
                                count_exec_.pause(false);
                              });

    holder_ += event_.connect("App:pending-update",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(ci::ColorA(0.6, 0.6, 0.6, 1));
                              });

    holder_ += event_.connect("App:resume-update",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(force_timeline_, 0.3, ci::ColorA(1, 1, 1, 1));
                              });

    // Transition
    static const std::vector<std::pair<const char*, const char*>> transition = {
      { "Credits:begin",  "Credits:Finished" },
      { "Settings:begin", "Settings:Finished" },
      { "Records:begin",  "Records:Finished" },
      { "Ranking:begin",  "Ranking:Finished" },
    };

    for (const auto& t : transition)
    {
      setTransitionEvent(t.first, t.second);
    }


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
    game_->preparationPlay(engine_);
    view_.setColor(ci::ColorA(1, 1, 1, 1));
  }


private:
  void resize(const Connection&, const Arguments&) noexcept
  {
    camera_.resize();
  }

	bool update(double current_time, double delta_time) noexcept override
  {
    // NOTICE pause中でもカウンタだけは進める
    //        pause→タイトルへ戻る演出のため
    count_exec_.update(delta_time);
    force_timeline_->step(delta_time);

    if (paused_)
    {
      return true;
    }

    game_->update(delta_time);
    fixed_exec_.update(delta_time);

    // カメラの中心位置変更
    target_position_ += (field_center_ - target_position_) * 0.025f;
    camera_distance_ += (field_distance_ - camera_distance_) * 0.05f;
    calcCamera(camera_.body());

    if (game_->isPlaying())
    {
      // 10秒切った時のカウントダウン判定
      auto play_time = game_->getPlayTime();
      u_int prev    = std::ceil(play_time + delta_time);
      u_int current = std::ceil(play_time);

      if ((current <= 10) && (prev != current))
      {
        game_event_.insert("Game:countdown");
      }

      // パネル設置操作
      if (touch_put_)
      {
        // タッチしたまま時間経過
        put_remaining_ -= delta_time;

        {
          auto ndc_pos = camera_.body().worldToNdc(cursor_pos_);
          auto scale = 1.0f - glm::clamp(float(put_remaining_ / current_putdown_time_), 0.0f, 1.0f);
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
          game_event_.insert("Panel:put");

          // 次のパネルの準備
          rotate_offset_.stop();
          rotate_offset_ = 0.0f;
          can_put_       = false;

          startNextPanelEase();

          touch_put_ = false;
          // 次のパネルを操作できないようにしとく
          draged_length_ = 100.0f;
            
          // Fieldの中心を再計算
          calcViewRange(true);
          calcNextPanelPosition();
        }
      }
    }

    put_gauge_timer_ += delta_time;

    timeline_->step(delta_time);

    // ゲーム内で起こったイベントをSoundに丸投げする
    if (!game_event_.empty())
    {
      Arguments args {
        { "event", game_event_ }
      };
      event_.signal("Game:Event", args);
      game_event_.clear();
    }

    return true;
  }


  void drawField() noexcept
  {
    // 本編
    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);
    ci::gl::disableAlphaBlending();
    ci::gl::setMatrices(camera_.body());
    ci::gl::clear(ci::Color(0, 0, 0));

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
          drawPanelEdge(panels_[game_->getHandPanel()], pos, game_->getHandRotation());
          
          // 置こうとしている場所の周囲
          auto around = game_->enumerateAroundPanels(field_pos_);
          if (!around.empty())
          {
            for (auto& it : around)
            {
              auto p = it.first * int(PANEL_SIZE);
              glm::vec3 disp_pos(p.x, 0.0f, p.y);

              auto status = it.second;
              drawPanelEdge(panels_[status.number], disp_pos, status.rotation);
            }
          }

          // パネルのAABB
          // auto aabb = view_.panelAabb(game_->getHandPanel());
          // aabb.transform(glm::translate(cursor_pos_));
          // ci::gl::color(0, 1, 0);
          // ci::gl::drawStrokedCube(aabb);
        }
      }
#endif
    }
    auto bg_pos = calcBgPosition();
    view_.drawFieldBg(bg_pos);

    view_.drawEffect();
  }

  void draw(const Connection&, const Arguments&) noexcept
  {
#if defined (DEBUG)
    if (debug_draw_) return;
#endif

    drawField();

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
      game_->moveHandPanel(grid_pos);

      // 少し宙に浮いた状態
      cursor_pos_ = glm::vec3(field_pos_.x * PANEL_SIZE, panel_height_, field_pos_.y * PANEL_SIZE);
      startMoveEase();
      game_event_.insert("Panel:move");
    }
  }

  // フィールドの広さから注視点と距離を計算
  // blank: true パネルが置ける場所を考慮する
  void calcViewRange(bool blank) noexcept
  {
    auto result = game_->getFieldCenterAndDistance(blank);

    auto center = result.first * float(PANEL_SIZE);

    {
      // ある程度の範囲が変更対象
      auto d = glm::distance(center, target_position_);
      // 見た目の距離に変換
      auto dd = d / camera_distance_;
      DOUT << "target_rate: " << dd << std::endl;
      if ((dd > target_rate_.x) && (dd < target_rate_.y))
      {
        field_center_.x = center.x;
        field_center_.z = center.z;
      }
    }
    
    auto d = result.second * PANEL_SIZE;
    const auto& camera = camera_.body();
    float fov_v = camera.getFov();
    float fov_h = camera.getFovHorizontal();
    float fov = (fov_v < fov_h) ? fov_v : fov_h;
    float distance = d / std::tan(ci::toRadians(fov * 0.5f));

    if (fov_v < fov_h)
    {
      // 横長画面の場合はカメラが斜め上から見下ろしているのを考慮
      float n = d / std::cos(camera_rotation_.x);
      distance -= n;
    }

    {
      // 一定値以上遠のく場合は「ユーザー操作で意図的に離れている」
      // と判断する
      auto rate = distance / camera_distance_;
      DOUT << "distace_rate: " << rate << std::endl;
      if ((rate > distance_rate_.x) && (rate < distance_rate_.y))
      {
        field_distance_ = distance;
      }
    }

    // 強制モード
    if (force_camera_)
    {
      field_center_.x = center.x;
      field_center_.z = center.z;
      field_distance_ = distance;
    }
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
    auto positions = game_->getBlankPositions();
    // 適当に並び替える
    std::shuffle(std::begin(positions), std::end(positions), engine_);

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

    touch_put_ = false;
    can_put_   = game_->canPutToBlank(field_pos_);
  }

  // パネル移動演出
  void startMoveEase() noexcept
  {
    panel_disp_pos_.stop();
    // 経過時間で移動速度が上がる
    // NOTICE プレイに影響は無い
    auto duration = glm::mix(disp_ease_duration_.x, disp_ease_duration_.y, game_->getPlayTimeRate());
    timeline_->apply(&panel_disp_pos_, cursor_pos_, duration, getEaseFunc(disp_ease_name_));
  }

  // パネル回転演出
  void startRotatePanelEase() noexcept
  {
    rotate_offset_.stop();
    rotate_offset_ = 90.0f;
    // 経過時間で回転速度が上がる
    // NOTICE プレイに影響は無い
    auto duration = glm::mix(rotate_ease_duration_.x, rotate_ease_duration_.y, game_->getPlayTimeRate());
    timeline_->apply(&rotate_offset_, 0.0f, duration, getEaseFunc(rotate_ease_name_));
  }


  // 次のパネルの出現演出
  void startNextPanelEase() noexcept
  {
    height_offset_.stop();
    height_offset_ = height_ease_start_;
    timeline_->apply(&height_offset_, 0.0f, height_ease_duration_, getEaseFunc(height_ease_name_));
  }


  // Game本体初期化
  void resetGame() noexcept
  {
    count_exec_.add(0.5,
                    [this]() noexcept
                    {
                      view_.clear();
                      view_.resetPauseEffect(force_timeline_);
                      timeline_->clear();
                      force_timeline_->clear();

                      paused_ = false;
                      count_exec_.pause(false);
                      fixed_exec_.clear();

                      // Game再生成
                      game_ = std::make_unique<Game>(params_["game"], event_, panels_);
                      game_->preparationPlay(engine_);
                                                  
                      field_center_   = glm::vec3();
                      field_distance_ = params_.getValueForKey<float>("field.camera.distance");
                    },
                    true);
  }

  // カメラから見える範囲のBGを計算
  glm::vec3 calcBgPosition() const noexcept
  {
    const auto& camera = camera_.body();
    auto aspect = camera.getAspectRatio();
    auto ray = camera.generateRay(0.5, 0.5, aspect);
    // 地面との交差位置を計算
    float z;
    ray.calcPlaneIntersection(glm::vec3(0, -5, 0), glm::vec3(0, 1, 0), &z);
    auto pos = ray.calcPosition(z);
    // World position→升目座標
    auto p = roundValue(pos.x, pos.z, PANEL_SIZE * 2);

    return { p.x * PANEL_SIZE * 2, 0, p.y * PANEL_SIZE * 2 };
  }


  // 画面遷移演出
  void setTransitionEvent(const std::string& begin, const std::string& end) noexcept
  {
    holder_ += event_.connect(begin,
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(force_timeline_, transition_duration_, transition_color_);
                              });
    holder_ += event_.connect(end,
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(force_timeline_, transition_duration_, ci::ColorA(1, 1, 1, 1));
                              });
  }

  // 結果を計算
  Score calcGameScore(const Arguments& args) noexcept
  {
    // ハイスコア判定
    auto high_score     = archive_.getRecord<u_int>("high-score");
    auto total_score    = boost::any_cast<u_int>(args.at("total_score"));
    auto total_ranking  = boost::any_cast<u_int>(args.at("total_ranking"));
    bool get_high_score = total_score > high_score;

    Score score = {
      boost::any_cast<const std::vector<u_int>&>(args.at("scores")),
      boost::any_cast<u_int>(args.at("total_score")),
      total_ranking,
      boost::any_cast<u_int>(args.at("total_panels")),

      boost::any_cast<u_int>(args.at("panel_turned_times")),
      boost::any_cast<u_int>(args.at("panel_moved_times")),

      game_->getLimitTime(),

      get_high_score
    };

    DOUT << "high: " << high_score << " total: " << total_score << std::endl;

    return score;
  }

  // Gameの記録
  void recordGameScore(const Score& score) noexcept
  {
    // 記録にとっとく
    auto path = std::string("game-") + getFormattedDate() + ".json";
    game_->save(path);
    // pathを記録
    auto game_json = ci::JsonTree::makeObject();
    game_json.addChild(ci::JsonTree("path",  path))
    .addChild(ci::JsonTree("score", score.total_score))
    .addChild(ci::JsonTree("rank",  score.total_ranking))
    ;

    // TIPS const参照からインスタンスを生成している
    auto json = archive_.getRecordArray("games");
    json.pushBack(game_json);
                                  
    Json::sort(json,
               [](const ci::JsonTree& a, const ci::JsonTree& b) noexcept
               {
                 auto score_a = a.getValueForKey<u_int>("score");
                 auto score_b = b.getValueForKey<u_int>("score");
                 return score_a < score_b;
               });
    // ランキング圏外を捨てる
    if (json.getNumChildren() > ranking_records_)
    {
      size_t count = json.getNumChildren() - ranking_records_;
      for (size_t i = 0; i < count; ++i)
      {
        auto p = json[ranking_records_].getValueForKey<std::string>("path");
        auto full_path = getDocumentPath() / p;
        // ファイルを削除
        try
        {
          DOUT << "remove: " << full_path << std::endl;
          ci::fs::remove(full_path);
        }
        catch (ci::fs::filesystem_error& ex)
        {
          DOUT << ex.what() << std::endl;
        }

        json.removeChild(ranking_records_);
      }
    }

    archive_.setRecordArray("games", json);
    archive_.recordGameResults(score);
  }


  // FIXME 変数を後半に定義する実験
  const ci::JsonTree& params_;

  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;
  FixedTimeExec fixed_exec_;

  Archive& archive_;
  
  std::mt19937 engine_;

  bool paused_ = false;
  bool prohibited_ = false;

  std::vector<Panel> panels_;
  std::unique_ptr<Game> game_;

  // パネル操作
  float draged_length_;
  float draged_max_length_;
  bool touch_put_;
  double put_remaining_;

  bool manipulated_ = false;

  float panel_height_;
  glm::vec2 putdown_time_;
  double current_putdown_time_;

  // 実際のパネル位置
  glm::vec3 cursor_pos_;

  // パネルを配置しようとしている位置
  glm::ivec2 field_pos_;
  // 配置可能
  bool can_put_ = false;

  // パネル位置
  ci::Anim<glm::vec3> panel_disp_pos_;
  glm::vec2 disp_ease_duration_;
  std::string disp_ease_name_;

  // パネルの回転
  ci::Anim<float> rotate_offset_ = 0.0f;
  glm::vec2 rotate_ease_duration_;
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

  // 距離を調整するための係数
  glm::vec2 target_rate_;
  glm::vec2 distance_rate_;
  // カメラ計算を優先
  bool force_camera_ = false;

  // Fieldの中心座標
  glm::vec3 field_center_;
  float field_distance_ = 0.0f;

  // 表示
  Camera camera_;
  View view_;

  // Pause
  glm::vec2 pause_duration_;
  std::string pause_ease_;

  // Tween用
  ci::TimelineRef timeline_;
  // Pauseでも続行する
  ci::TimelineRef force_timeline_;

  // ゲーム内で発生したイベント
  std::set<std::string> game_event_;

  // Rankingで表示する数
  u_int ranking_records_;

  // Transition
  float transition_duration_;
  ci::ColorA transition_color_;


#ifdef DEBUG
  bool disp_debug_info_ = false;
  bool debug_draw_ = false;

  // Fieldの外接球
  ci::Sphere framing_sphere_;
#endif
};

}
