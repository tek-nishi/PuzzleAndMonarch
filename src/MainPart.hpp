#pragma once

//
// アプリ本編
//

#include "Defines.hpp"
#include <tuple>
#include <limits>
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
      bg_height_(params_.getValueForKey<float>("field.bg_height")),
      view_(params["field"]),
      ranking_records_(params.getValueForKey<u_int>("game.ranking_records")),
      transition_duration_(params.getValueForKey<float>("ui.transition.duration")),
      transition_color_(Json::getColorA<float>(params["ui.transition.color"]))
  {
    initial_camera_rotation_ = camera_rotation_;
    initial_camera_distance_ = camera_distance_;
    initial_target_position_ = target_position_;

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
                                if (paused_ || prohibited_) return;

                                draged_length_ = 0.0f;
                                manipulated_   = false;
                                touch_put_     = false;

                                if (!game_->isPlaying()) return;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                // 元々パネルのある位置をタップ→長押しで設置
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

    holder_ += event_.connect("multi_touch_began",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (touch_put_)
                                {
                                  // シングルタッチ操作解除
                                  touch_put_ = false;
                                  event_.signal("Game:PutEnd", Arguments());
                                }
                              });

    holder_ += event_.connect("single_touch_moved",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                if (paused_ || prohibited_) return;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                // NOTICE iOSはtouchと同時にmoveも呼ばれる
                                //        glm::distanceは距離０の時にinfを返すので、期待した結果にならない
                                float e = std::numeric_limits<float>::epsilon();
                                if (glm::distance2(touch.pos, touch.prev_pos) < e) return;

                                // ドラッグの距離を調べて、タップかドラッグか判定
                                draged_length_ += glm::distance(touch.pos, touch.prev_pos);
                                manipulated_ = draged_length_ > draged_max_length_;
                                if (touch_put_ && manipulated_)
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
                                if (!game_->isPlaying() || paused_ || prohibited_) return;

                                const auto& touch = boost::any_cast<const Touch&>(arg.at("touch"));
                                if (touch.handled) return;

                                // 画面回転操作をした場合、パネル操作は全て無効
                                if (manipulated_)
                                {
                                  DOUT << "draged_length: " << draged_length_ << std::endl;
                                  return;
                                }

                                // 設置操作無効
                                if (touch_put_)
                                {
                                  event_.signal("Game:PutEnd", Arguments());
                                  touch_put_ = false;
                                }
                                
                                if (isCursorPos(touch.pos))
                                {
                                  // パネルを回転
                                  game_->rotationHandPanel();
                                  startRotatePanelEase();
                                  can_put_ = game_->canPutToBlank(field_pos_);
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
                                auto l0 = glm::distance2(touches[0].pos, touches[1].pos);
                                auto l1 = glm::distance2(touches[0].prev_pos, touches[1].prev_pos);
                                if (l0 > 0.0f && l1 > 0.0f)
                                {
                                  l0 = std::sqrt(l0);
                                  l1 = std::sqrt(l1);

                                  auto dl = l0 - l1;
                                  if (std::abs(dl) > 1.0f)
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
                                }
                              });

    // 各種イベント
    holder_ += event_.connect("Title:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                game_event_.insert("Game:ready");

                                auto delay = params_.getValueForKey<float>("ui.transition.game_begin_delay");
                                view_.setColor(transition_duration_, transition_color_, delay);
                                count_exec_.add(params_.getValueForKey<float>("ui.transition.game_begin_duration"),
                                                [this]() noexcept
                                                {
                                                  view_.setColor(transition_duration_, ci::ColorA::white());
                                                });

                                field_distance_ = initial_camera_distance_;
                                field_center_   = initial_target_position_;
                                // auto option = timeline_->applyPtr(&camera_rotation_,
                                //                                   initial_camera_rotation_, 0.8, getEaseFunc("OutCubic"));
                                // option.finishFn([this]() noexcept
                                //                 {
                                //                   prohibited_ = false;
                                //                 });
                                prohibited_ = true;
                                count_exec_.add(1.0,
                                                [this]() noexcept
                                                {
                                                  prohibited_ = false;
                                                });
                                game_->updateGameUI();
                              });

    holder_ += event_.connect("Game:Start",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                game_->beginPlay();
                                calcNextPanelPosition();
                              });

    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // 中断
                                archive_.addRecord("abort-times", uint32_t(1));
                                archive_.save();
                                count_exec_.add(params_.getValueForKey<double>("field.game_abort_delay"),
                                                [this]() noexcept
                                                {
                                                  resetGame();
                                                  resetCamera();
                                                },
                                                true);
                              });

    holder_ += event_.connect("Game:PutPanel",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                auto panel      = boost::any_cast<int>(args.at("panel"));
                                const auto& pos = boost::any_cast<glm::ivec2>(args.at("field_pos"));
                                auto rotation   = boost::any_cast<u_int>(args.at("rotation"));
                                view_.addPanel(panel, pos, rotation);
                                view_.startPutEase(game_->getPlayTimeRate());
                                view_.updateBlank(game_->getBlankPositions());
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
                                view_.setColor(transition_duration_, transition_color_);

                                // スコア計算
                                auto score      = calcGameScore(args);
                                bool high_score = isHighScore(score);
                                recordGameScore(score, high_score);
                                // ランクインしてるか調べる
                                auto rank_in = isRankIn(score.total_score);
                                auto ranking = getRanking(score.total_score);

                                count_exec_.add(params_.getValueForKey<double>("field.result_begin_delay"),
                                                [this, score, rank_in, ranking, high_score]() noexcept
                                                {
                                                  Arguments a {
                                                    { "score",      score },
                                                    { "rank_in",    rank_in },
                                                    { "ranking",    ranking },
                                                    { "high_score", high_score }
                                                  };
                                                  event_.signal("Result:begin", a);
                                                  view_.setColor(transition_duration_, ci::ColorA::white());
                                                });

                                count_exec_.add(params_.getValueForKey<double>("field.auto_camera_duration"),
                                                [this]() noexcept
                                                {
                                                  force_camera_ = false;
                                                  prohibited_   = false;
                                                });

                                auto speed = params_.getValueForKey<double>("field.auto_camera_rotation_speed");
                                fixed_exec_.add(params_.getValueForKey<double>("field.auto_camera_delay"),
                                                -1.0,
                                                [this, speed](double delta_time) noexcept
                                                {
                                                  camera_rotation_.y += M_PI * speed * delta_time;
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
                                   view_.startEffect(p);
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
                                   view_.startEffect(p);
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
                                 view_.startEffect(p);
                               }
                               game_event_.insert("Comp:church");
                             });

    // Result→Title
    holder_ += event.connect("Result:Finished",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                // NOTICE TOP10入りの時はResult→Ranking→Titleと遷移する
                                bool rank_in = boost::any_cast<bool>(args.at("rank_in"));
                                if (rank_in)
                                {
                                  view_.setColor(transition_duration_, transition_color_);
                                }
                                else
                                {
                                  count_exec_.add(params_.getValueForKey<double>("field.reset_delay"),
                                                  [this]() noexcept
                                                  {
                                                    resetGame();
                                                    resetCamera();
                                                  },
                                                  true);
                                }
                              });

    // Ranking
    holder_ += event_.connect("Ranking:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(params_.getValueForKey<double>("field.reset_delay"),
                                                [this]() noexcept
                                                {
                                                  force_camera_ = true;
                                                  manipulated_  = false;

                                                  // TOPの記録を読み込む
                                                  {
                                                    const auto& json = archive_.getRecordArray("games");
                                                    if (json.hasChildren() && json[0].hasChild("path"))
                                                    {
                                                      view_.clear();
                                                      game_->load(json[0].getValueForKey("path"));
                                                      calcViewRange(false);
                                                    }
                                                  }

                                                  count_exec_.add(params_.getValueForKey<double>("field.auto_camera_duration"),
                                                                  [this]() noexcept
                                                                  {
                                                                    force_camera_ = false;
                                                                  });

                                                  auto speed = params_.getValueForKey<double>("field.auto_camera_rotation_speed");
                                                  fixed_exec_.add(params_.getValueForKey<double>("field.auto_camera_delay"),
                                                                  -1.0,
                                                                  [this, speed](double delta_time) noexcept
                                                                  {
                                                                    camera_rotation_.y += M_PI * speed * delta_time;
                                                                    calcCamera(camera_.body());

                                                                    return !manipulated_;
                                                                  });
                                                });
                              });
    
    // Ranking詳細開始
    holder_ += event_.connect("view:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(transition_duration_, ci::ColorA::white(), 0.8f);
                              });
    
    holder_ += event_.connect("back:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(transition_duration_, transition_color_, 0.8f);
                              });

    holder_ += event_.connect("Ranking:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(params_.getValueForKey<double>("field.reset_delay"),
                                                [this]() noexcept
                                                {
                                                  resetGame();
                                                  resetCamera();
                                                });
                              });

    // System
    holder_ += event_.connect("App:ResignActive",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                if (game_->isPlaying() && !paused_)
                                {
                                  // 自動ポーズ
                                  event_.signal("pause:touch_ended", Arguments());
                                }
                              });

    holder_ += event_.connect("pause:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // Pause開始
                                paused_ = true;
                                count_exec_.pause();
                                count_exec_.add(params_.getValueForKey<double>("field.pause_exec_delay"),
                                                [this]() noexcept
                                                {
                                                  view_.pauseGame();
                                                },
                                                true);

                                view_.setColor(transition_duration_, transition_color_,
                                               params_.getValueForKey<double>("field.pause_transition_delay"));
                              });

    holder_ += event_.connect("resume:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // ゲーム続行
                                count_exec_.add(params_.getValueForKey<double>("field.pause_exec_delay"),
                                                [this]() noexcept
                                                {
                                                  view_.resumeGame();
                                                },
                                                true);

                                view_.setColor(transition_duration_, ci::ColorA::white(),
                                               params_.getValueForKey<double>("field.pause_transition_delay"));
                              });

    holder_ += event_.connect("abort:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // ゲーム終了
                                view_.setColor(transition_duration_, ci::ColorA::white(),
                                               params_.getValueForKey<double>("field.pause_transition_delay"));
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
                                auto color = Json::getColorA<float>(params_["field.pending_update_color"]);
                                view_.setColor(color);
                              });

    holder_ += event_.connect("App:resume-update",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                auto duration = params_.getValueForKey<float>("field.pending_update_duration");
                                view_.setColor(duration, ci::ColorA::white());
                              });

    holder_ += event_.connect("Settings:Trash",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // 記録を消去
                                eraseRecords();
                              });
    
    // Transition
    static const std::vector<std::pair<const char*, const char*>> transition = {
      { "Credits:begin",  "Credits:Finished" },
      { "Settings:begin", "Settings:Finished" },
      { "Records:begin",  "Records:Finished" },
      { "Ranking:begin",  "Ranking:Finished" },
    };

    auto transition_delay = params.getValueForKey<double>("ui.transition.delay");
    for (const auto& t : transition)
    {
      setTransitionEvent(t.first, t.second, transition_delay);
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
    
    holder_ += event_.connect("debug-shadowmap",
                              [this](const Connection&, Arguments& arg) noexcept
                              {
                                disp_shadowmap_ = !disp_shadowmap_;
                              });
#endif
    // 本編準備
    game_->preparationPlay(engine_);
    view_.setColor(ci::ColorA::white());
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

    view_.update(delta_time, paused_);

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

        if (put_remaining_ < 0.0)
        {
          game_->putHandPanel(field_pos_);
          event_.signal("Game:PutEnd", Arguments());
          game_event_.insert("Panel:put");

          // 次のパネルの準備
          can_put_   = false;
          touch_put_ = false;
          // 次のパネルを操作できないようにしとく
          manipulated_ = true;
          // 適当に大きな値
          draged_length_ = draged_max_length_ * 2;
            
          // Fieldの中心を再計算
          calcViewRange(true);
          // 次のパネル
          calcNextPanelPosition();
        }
      }
    }

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


#if 0
  void drawField() noexcept
  {
    // 本編
    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);
    ci::gl::disableAlphaBlending();

    drawShadow();

    ci::gl::setMatrices(camera_.body());
    ci::gl::clear(ci::Color::black());

    // フィールド
    ci::gl::ScopedGlslProg prog(view_.setupFieldShader(light_camera_));
    ci::gl::ScopedTextureBind texScope(shadow_map_, 0);
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
#endif


  void draw(const Connection&, const Arguments&) noexcept
  {
#if defined (DEBUG)
    if (debug_draw_) return;
#endif

    View::Info info {
      game_->isPlaying(),
      can_put_,

      game_->getHandPanel(),
      game_->getHandRotation(),

      field_pos_,

      calcBgPosition(),

      &camera_.body()
    };

    view_.drawField(info);

#if defined(DEBUG)
    if (disp_shadowmap_)
    {
      view_.drawShadowMap();
    }

#if 0
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

    view_.setupShadowCamera(map_center_);
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
      startMovePanelEase();
      game_event_.insert("Panel:move");
    }
  }

  // フィールドの広さから注視点と距離を計算
  // blank: true パネルが置ける場所を考慮する
  void calcViewRange(bool blank) noexcept
  {
    auto result = game_->getFieldCenterAndDistance(blank);

    map_center_ = result.first * float(PANEL_SIZE);

    {
      // ある程度の範囲が変更対象
      auto d = glm::distance(map_center_, target_position_);
      // 見た目の距離に変換
      auto dd = d / camera_distance_;
      DOUT << "target_rate: " << dd << std::endl;
      if ((dd > target_rate_.x) && (dd < target_rate_.y))
      {
        field_center_.x = map_center_.x;
        field_center_.z = map_center_.z;
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
      field_center_.x = map_center_.x;
      field_center_.z = map_center_.z;
      field_distance_ = ci::clamp(distance, 
                                  camera_distance_range_.x, camera_distance_range_.y);
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
    view_.setPanelPosition(cursor_pos_);
    view_.startNextPanelEase();

    touch_put_ = false;
    can_put_   = game_->canPutToBlank(field_pos_);
  }

  // パネル移動演出
  void startMovePanelEase() noexcept
  {
    // 経過時間で移動速度が上がる
    // NOTICE プレイに影響は無い
    view_.startMovePanelEase(cursor_pos_, game_->getPlayTimeRate());
  }

  // パネル回転演出
  void startRotatePanelEase() noexcept
  {
    // // 経過時間で回転速度が上がる
    // // NOTICE プレイに影響は無い
    view_.startRotatePanelEase(game_->getPlayTimeRate());
  }

  // Game本体初期化
  void resetGame() noexcept
  {
    view_.clear();

    paused_ = false;
    count_exec_.pause(false);
    fixed_exec_.clear();

    // Game再生成
    game_.reset();            // TIPS メモリを２重に確保したくないので先にresetする
    game_ = std::make_unique<Game>(params_["game"], event_, panels_);
    game_->preparationPlay(engine_);
                                                  
    field_center_   = initial_target_position_;
    field_distance_ = initial_camera_distance_;
  }

  // カメラから見える範囲のBGを計算
  glm::vec3 calcBgPosition() const noexcept
  {
    const auto& camera = camera_.body();
    auto aspect = camera.getAspectRatio();
    auto ray = camera.generateRay(0.5, 0.5, aspect);
    // 地面との交差位置を計算
    float z;
    ray.calcPlaneIntersection(glm::vec3(0, bg_height_, 0), glm::vec3(0, 1, 0), &z);
    return ray.calcPosition(z);
  }

  // カメラリセット
  void resetCamera() noexcept
  {
    force_camera_ = false;

    camera_rotation_ = initial_camera_rotation_;
    camera_distance_ = initial_camera_distance_;
    target_position_ = initial_target_position_;
  }


  // 画面遷移演出
  void setTransitionEvent(const std::string& begin, const std::string& end, double delay) noexcept
  {
    holder_ += event_.connect(begin,
                              [this, delay](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(transition_duration_, transition_color_, delay);
                              });
    holder_ += event_.connect(end,
                              [this, delay](const Connection&, const Arguments&) noexcept
                              {
                                view_.setColor(transition_duration_, ci::ColorA::white(), delay);
                              });
  }

  // 結果を計算
  Score calcGameScore(const Arguments& args) const noexcept
  {
    Score score = {
      boost::any_cast<const std::vector<u_int>&>(args.at("scores")),
      boost::any_cast<u_int>(args.at("total_score")),
      boost::any_cast<u_int>(args.at("total_ranking")),
      boost::any_cast<u_int>(args.at("total_panels")),

      boost::any_cast<bool>(args.at("perfect")),

      boost::any_cast<u_int>(args.at("panel_turned_times")),
      boost::any_cast<u_int>(args.at("panel_moved_times")),

      game_->getLimitTime(),
    };

    return score;
  }

  bool isHighScore(const Score& score) const noexcept
  {
    // ハイスコア判定
    auto high_score = archive_.getRecord<u_int>("high-score");
    return score.total_score > high_score;
  }

  // Gameの記録
  void recordGameScore(const Score& score, bool high_score) noexcept
  {
    archive_.setRecord("saved", true); 
    
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
        if (json[ranking_records_].hasChild("path"))
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
        }

        json.removeChild(ranking_records_);
      }
    }
    archive_.setRecordArray("games", json);

    archive_.recordGameResults(score, high_score);
  }

  bool isRankIn(u_int score) const noexcept
  {
    const auto& ranking = archive_.getRecordArray("games");
    for (const auto& r : ranking)
    {
      if (score == r.getValueForKey<u_int>("score")) return true;
    }
    return false;
  }

  u_int getRanking(u_int score) const noexcept
  {
    const auto& ranking = archive_.getRecordArray("games");
    u_int i;
    for (i = 0; i < ranking.getNumChildren(); ++i)
    {
      if (score == ranking[i].getValueForKey<u_int>("score")) return i;
    }

    // NOTICE 見つからない場合は最大値
    return std::numeric_limits<u_int>::max();
  }


  // 記録の消去
  void eraseRecords() noexcept
  {
    // 保存してあるプレイ結果を削除
    auto game_records = archive_.getRecordArray("games");
    for (const auto& game : game_records)
    {
      if (game.hasChild("path"))
      {
        auto p = game.getValueForKey<std::string>("path");
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
      }
    }

    archive_.erase();
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
  // true: カメラ操作不可
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

  // パネル位置
  glm::vec3 cursor_pos_;

  // パネルを配置しようとしている位置
  glm::ivec2 field_pos_;
  // 配置可能
  bool can_put_ = false;

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
  glm::vec3 map_center_;
  glm::vec3 field_center_;
  float field_distance_ = 0.0f;

  // カメラを初期状態に戻すための変数
  glm::vec2 initial_camera_rotation_;
  float initial_camera_distance_;
  glm::vec3 initial_target_position_;

  // 地面の高さ 
  float bg_height_;

  // 表示
  Camera camera_;
  View view_;

  // ゲーム内で発生したイベント(音効用)
  std::set<std::string> game_event_;

  // Rankingで表示する数
  u_int ranking_records_;

  // TODO Viewへ移動(Transition)
  float transition_duration_;
  ci::ColorA transition_color_;


#ifdef DEBUG
  bool disp_debug_info_ = false;
  bool debug_draw_      = false;
  bool disp_shadowmap_  = false;

  // Fieldの外接球
  ci::Sphere framing_sphere_;
#endif
};

}
