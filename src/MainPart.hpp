#pragma once

//
// アプリ本編
//

#include "Defines.hpp"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cinder/Rand.h>
#include <cinder/Camera.h>
#include <cinder/Ray.h>
#include <cinder/Timer.h>
#include "Params.hpp"
#include "JsonUtil.hpp"
#include "Game.hpp"
#include "Counter.hpp"
#include "Font.hpp"
#include "View.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "ConnectionHolder.hpp"
#include "UICanvas.hpp"


namespace ngs {

class MainPart
{

public:
  MainPart(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : event_(event),
      params_(params),
      panels(createPanels()),
      game(std::make_unique<Game>(panels)),
      rotation(toRadians(Json::getVec<glm::vec2>(params["field_camera.rotation"]))),
      distance(params.getValueForKey<float>("field_camera.distance")),
      field_camera(params["field_camera"]),
      view(createView()),
      drawer_(params["ui"]),
      canvas_(createCanvas("ui.camera", "title.canvas.widgets"))
  {
    // フィールドカメラ
    auto& camera = field_camera.body();
    glm::quat q(glm::vec3(rotation.x, rotation.y, 0));
    glm::vec3 p = q * glm::vec3{ 0, 0, -distance };
    camera.lookAt(p, glm::vec3(0));

    eye_position    = camera.getEyePoint();
    target_position = glm::vec3(0);

    // 各種イベント
    holder_ += event.connect("start:touch_ended",
                             [this](const Connection& connection, Arguments& arg) noexcept
                             {
                               // ゲーム開始
                               game->beginPlay();
                               playing_mode = GAMESTART;
                               hight_offset = 500.0f;

                               game_score = game->getScores();
                               game_score_effect.resize(game_score.size());
                               std::fill(std::begin(game_score_effect), std::end(game_score_effect), 0);

                               counter.add("gamestart", 90);

                               connection.disconnect();
                             });


    holder_ += event_.connect("multi_touch_moved",
                              [this](const Connection&, const Arguments& arg) noexcept
                              {
                                const auto& touches = boost::any_cast<const std::vector<Touch>&>(arg.at("touches"));
                                auto& camera = field_camera.body();

                                float l      = glm::distance(touches[0].pos, touches[1].pos);
                                float prev_l = glm::distance(touches[0].prev_pos, touches[1].prev_pos);
                                float dl = l - prev_l;
                                if (std::abs(dl) > 1.0f)
                                {
                                  // ピンチング
                                  distance = std::max(distance - dl * 0.25f, camera.getNearClip() + 1.0f);

                                  glm::quat q(glm::vec3(rotation.x, rotation.y, 0));
                                  glm::vec3 p = q * glm::vec3{ 0, 0, -distance };
                                  camera.lookAt(p + target_position, target_position);
                                  eye_position = camera.getEyePoint();
                                }
                                else
                                {
                                  // 平行移動
                                  auto v = touches[0].pos - touches[0].prev_pos;

                                  glm::quat q(glm::vec3(0, rotation.y, 0));
                                  glm::vec3 p = q * glm::vec3(v.x, 0, v.y);
                                  target_position += p;
                                  eye_position += p;
                                  camera.setEyePoint(eye_position);
                                }
                              });
  }


#if 0
  glm::vec2 touch_pos;
  bool draged = false;

  void touchBegan(glm::vec2 pos) noexcept
  {
    touch_pos = pos;
    draged = false;
  }

  void touchMoved(glm::vec2 pos) noexcept
  {
    // マウスの移動ベクトルに垂直なベクトル→回転軸
    glm::vec2 d = pos - touch_pos;
    glm::vec3 axis(-d.y, -d.x, 0.0f);
    float l = glm::length(d);

    if (l > 0.0f)
    {
      draged = true;

      auto& camera = field_camera.body();

      glm::quat q = glm::angleAxis(l * 0.002f, axis / l);
      auto cam_q = camera.getOrientation();
      auto cam_iq = glm::inverse(cam_q);

      auto v = eye_position - target_position;
      v = cam_q * q * cam_iq * v;
      eye_position = v + target_position;

      camera.setEyePoint(eye_position);
      camera.setOrientation(cam_q * q);

      touch_pos = pos;
    }
  }

  void touchEnded(glm::vec2 pos) noexcept
  {
    if (!draged)
    {

    }
  }
#endif


  void mouseMove(ci::app::MouseEvent event)
  {
#if 0
    auto pos = event.getPos();
    calcFieldPos(pos);
#endif
  }

	void mouseDown(ci::app::MouseEvent event)
  {
#if 0
    if (!event.isLeft()) return;

    mouse_draged   = false;
    left_click_pos = event.getPos();
    

    // camera_ui.mouseDown(event);
#endif
  }
  
	void mouseDrag(ci::app::MouseEvent event)
  {
#if 0
    if (!event.isLeftDown()) return;

    // クリック位置から3pixel程度の動きは無視
    glm::vec2 pos = event.getPos();
    if (glm::distance2(left_click_pos, pos) < 9.0f) return;

    mouse_draged = true;
    // camera_ui.mouseDrag(event);
#endif
  }
  
	void mouseUp(ci::app::MouseEvent event)
  {
#if 0
    switch (playing_mode) {
    case TITLE:
      if (event.isLeft() && !mouse_draged) {
        // ゲーム開始
        game->beginPlay();
        playing_mode = GAMESTART;
        hight_offset = 500.0f;

        game_score = game->getScores();
        game_score_effect.resize(game_score.size());
        std::fill(std::begin(game_score_effect), std::end(game_score_effect), 0);

        counter.add("gamestart", 90);
      }
      break;

    case GAMESTART:
    case GAMEMAIN:
      if (game->isPlaying()) {
        if (event.isLeft() && !mouse_draged) {
          // パネルを配置
          if (can_put) {
            game->putHandPanel(field_pos);
            rotate_offset = 0.0f;
            hight_offset  = 500.0f;
            can_put = false;
          }
        }
        else if (event.isRight()) {
          // パネルを回転
          game->rotationHandPanel();
          rotate_offset = 90.0f;
          can_put = game->canPutToBlank(field_pos);
        }
      }
      break;

    case GAMEEND:
      {
      }
      break;

    case RESULT:
      if (event.isLeft() && !mouse_draged) {
        // 再ゲーム
        game = std::make_unique<Game>(panels);
        playing_mode = TITLE;
      }
      break;
    }
#endif
  }

  void mouseWheel(ci::app::MouseEvent event)
  {
    // camera_ui.mouseWheel(event);
  }
  
  void keyDown(ci::app::KeyEvent event)
  {
    int code = event.getCode();
    pressing_key.insert(code);

#ifdef DEBUG
    // if (code == ci::app::KeyEvent::KEY_r) {
    //   // 強制リセット
    //   game = std::make_unique<ngs::Game>(panels);
    //   playing_mode = TITLE;
    // }
    if (code == ci::app::KeyEvent::KEY_t) {
      // 時間停止
      game->pauseTimeCount();
    }
    if (code == ci::app::KeyEvent::KEY_e) {
      // 強制終了
      game->endPlay();
    }

    if (code == ci::app::KeyEvent::KEY_d) {
      disp_debug_info = !disp_debug_info;
    }

    // 手持ちパネル強制変更
    if (code == ci::app::KeyEvent::KEY_b) {
      game->changePanelForced(-1);
    }
    else if (code == ci::app::KeyEvent::KEY_n) {
      game->changePanelForced(1);
    }
#endif

    switch (playing_mode) {
    case GAMEMAIN:
      if (game->isPlaying()) {
        // 強制的に次のカードを引く
        if (code == ci::app::KeyEvent::KEY_f) {
          game->forceNextHandPanel();
          rotate_offset = 0.0f;
          hight_offset  = 500.0f;
          can_put = false;
        }
      }
      break;
    }
  }

  void keyUp(ci::app::KeyEvent event)
  {
    int code = event.getCode();
    pressing_key.erase(code);
  }
  

	void update()
  {
    counter.update();
    auto current_time = game_timer.getSeconds();
    double delta_time = current_time - last_time;
    game->update(delta_time);

    last_time = current_time;

    // カメラの中心位置変更
    auto center_pos = game->getFieldCenter() * float(ngs::PANEL_SIZE);
    camera_center += (center_pos - camera_center) * 0.05f;

    switch (playing_mode) {
    case TITLE:
      {
      }
      break;

    case GAMESTART:
      {
        if (!counter.check("gamestart")) {
          playing_mode = GAMEMAIN;
          game_timer.start(0);
          last_time = 0.0;
                               
          canvas_.reset();
          canvas_ = createCanvas("ui.camera", "gamemain.canvas.widgets");
          DOUT << "GAMEMAIN." << std::endl;
        }
      }
      break;

    case GAMEMAIN:
      if (!game->isPlaying()) {
        // 結果画面へ
        playing_mode = GAMEEND;
        game_timer.stop();
        counter.add("gameend", 120);
      }
      break;

    case GAMEEND:
      {
        if (!counter.check("gameend")) {
          game_score = game->getScores();
          std::fill(std::begin(game_score_effect), std::end(game_score_effect), 0);
          playing_mode = RESULT;
          DOUT << "RESULT." << std::endl;
        }
      }
      break;

    case RESULT:
      {
      }
      break;
    }

    frame_counter += 1;
  }


  void resize() noexcept
  {
    field_camera.resize();
    canvas_->resize();

    DOUT << "resize: " << ci::app::getWindowSize() << std::endl;
  }


	void draw(glm::ivec2 window_size) noexcept
  {
    ci::gl::clear(ci::Color(0, 0, 0));
    
    // 本編
    ci::gl::disableAlphaBlending();

    // プレイ画面
    ci::gl::setMatrices(field_camera.body());
    ci::gl::translate(-camera_center.x, -5.0, -camera_center.y);

    ci::gl::enableDepth();
    ci::gl::enable(GL_CULL_FACE);

    // フィールド
    const auto& field_panels = game->getFieldPanels();
    ngs::drawFieldPanels(field_panels, view);

    if (game->isPlaying()) {
      // 置ける場所
      const auto& blank = game->getBlankPositions();
      ngs::drawFieldBlank(blank, view);
      
      // 手持ちパネル
      rotate_offset *= 0.8f;
      hight_offset  *= 0.8f;
      glm::vec3 pos(cursor_pos.x, cursor_pos.y + hight_offset, cursor_pos.z);
      ngs::drawPanel(game->getHandPanel(), pos, game->getHandRotation(), view, rotate_offset);
      
      if (can_put) {
        float s = std::abs(std::sin(frame_counter * 0.1)) * 0.1;
        glm::vec3 scale(0.9 + s, 1, 0.9 + s);
        drawFieldSelected(field_pos, scale, view);
        
        scale.x = 1.0 + s;
        scale.z = 1.0 + s;
        drawCursor(pos, scale, view);
      }

#ifdef DEBUG
      if (disp_debug_info) {
        // 手元のパネル
        ngs::drawPanelEdge(panels[game->getHandPanel()], pos, game->getHandRotation());

        // 置こうとしている場所の周囲
        auto around = game->enumerateAroundPanels(field_pos);
        if (!around.empty()) {
          for (auto it : around) {
            auto p = it.first * int(ngs::PANEL_SIZE);
            glm::vec3 disp_pos(p.x, 0.0f, p.y);

            auto status = it.second;
            ngs::drawPanelEdge(panels[status.number], disp_pos, status.rotation);
          }
        }
      }
#endif
    }
    drawFieldBg(view);

    // UI
    {
      ci::gl::enableDepth(false);
      ci::gl::disable(GL_CULL_FACE);
      ci::gl::enableAlphaBlending();

      canvas_->draw();
    }

#if 0
    ci::gl::setMatrices(ui_camera);

    {
      ci::gl::ScopedGlslProg prog(shader_font);

      switch (playing_mode) {
      case TITLE:
        {
          {
            font.size(100);
            std::string text("Apprentice Monarch");
            auto size = font.drawSize(text);
            font.draw(text, glm::vec2(-size.x / 2, 150), ci::ColorA(1, 1, 1, 1));
          }
          {
            font.size(60);
            std::string text("Left click to start");
            auto size = font.drawSize(text);

            float r = frame_counter * 0.05f;
            font.draw(text, glm::vec2(-size.x / 2, -300), ci::ColorA(1, 1, 1, (std::sin(r) + 1.0f) * 0.5f));
          }
        }
        break;

      case GAMESTART:
        {
          font.size(90);
          const char* text = "Secure the territory!";

          auto size = font.drawSize(text);
          font.draw(text, glm::vec2(-size.x / 2.0f, -size.y / 2.0f), ci::ColorA(1, 1, 1, 1));
        }
        break;

      case GAMEMAIN:
        {
          // 残り時間
          font.size(80);

          char text[100];
          u_int remainig_time = std::ceil(game->getRemainingTime());
          u_int minutes = remainig_time / 60;
          u_int seconds = remainig_time % 60;
          sprintf(text, "%d'%02d", minutes, seconds);

          // 時間が10秒切ったら色を変える
          ci::ColorA color(1, 1, 1, 1);
          if (remainig_time <= 10) {
            color = ci::ColorA(1, 0, 0, 1);
          }

          int hh = ci::app::getWindowHeight() / 2;

          font.draw(text, glm::vec2(remain_time_x, hh - 100), color);
        }
        {
          int hw = ci::app::getWindowWidth() / 2;
          drawGameInfo(25, glm::vec2(-hw + 50, 0), -40);
        }
        break;

      case GAMEEND:
        {
          font.size(90);
          const char* text = "Well done!";

          auto size = font.drawSize(text);
          font.draw(text, glm::vec2(-size.x / 2.0f, -size.y / 2.0f), ci::ColorA(1, 1, 1, 1));
        }
        break;

      case RESULT:
        {
          {
            font.size(80);
            std::string text("Result");
            auto size = font.drawSize(text);
            font.draw(text, glm::vec2(-size.x / 2, 250), ci::ColorA(1, 1, 1, 1));
          }
          {
            font.size(40);
            std::string text("Left click to title");
            auto size = font.drawSize(text);

            int hh = ci::app::getWindowHeight() / 2;
            float r = frame_counter * 0.05f;
            font.draw(text, glm::vec2(-size.x / 2, -hh + 50), ci::ColorA(1, 1, 1, (std::sin(r) + 1.0f) * 0.5f));
          }
          drawResult();
        }
        break;
      }
    }
#endif
  }


  void calcFieldPos(glm::vec2 pos) {
    // 画面奥に伸びるRayを生成
    ci::Ray ray = field_camera.body().generateRay(pos, ci::app::getWindowSize());

    auto m = glm::translate(glm::vec3{ camera_center.x, 5, camera_center.y });
    auto origin = m * glm::vec4(ray.getOrigin(), 1);
    ray.setOrigin(origin);

    // 地面との交差を調べ、正確な位置を計算
    float z;
    float on_field = ray.calcPlaneIntersection(glm::vec3(0), glm::vec3(0, 1, 0), &z);
    can_put = false;
    if (on_field) {
      cursor_pos = ray.calcPosition(z);
      
      field_pos.x = roundValue(cursor_pos.x, PANEL_SIZE);
      field_pos.y = roundValue(cursor_pos.z, PANEL_SIZE);
      can_put = game->canPutToBlank(field_pos);

      // 多少違和感を軽減するために位置を再計算
      ray.calcPlaneIntersection(glm::vec3(0, 10, 0), glm::vec3(0, 1, 0), &z);
      cursor_pos = ray.calcPosition(z);
    }
  }

#if 0
  // プレイ情報を表示
  void drawGameInfo(int font_size, glm::vec2 pos, float next_y) {
    jpn_font.size(font_size);

    const auto& scores = game->getScores();
    // 変動した箇所の色を変える演出用
    bool changed = false;
    for (size_t i = 0; i < scores.size(); ++i) {
      if (game_score[i] != scores[i]) {
        game_score_effect[i] = 60;
        changed = true;
      }
      game_score[i] = scores[i];
    }
    if (changed) {
      // 点が入った
    }

    for (auto& i : game_score_effect) {
      i = std::max(i - 1, 0);
    }

    const char* text[] = {
      u8"道の数:   %d",
      u8"道の長さ: %d",
      u8"森の数:   %d",
      u8"森の広さ: %d",
      u8"深い森:   %d",
      u8"街の数:   %d",
      u8"教会の数: %d",
      // u8"城の数:   %d",
    };

    u_int i = 0;
    for (const auto t : text) {
      ci::ColorA col = game_score_effect[i] ? ci::ColorA(1, 0, 0, 1)
                                            : ci::ColorA(1, 1, 1, 1);

      char buffer[100];
      sprintf(buffer, t, game_score[i]);
      jpn_font.draw(buffer, pos,col);

      pos.y += next_y;
      i += 1;
    }
  }

  // 結果を表示
  void drawResult() {
    drawGameInfo(30, glm::vec2(-300, 150), -50);

    int score   = game->getTotalScore();
    int ranking = game->getTotalRanking();

    const char* ranking_text[] = {
      "Emperor",
      "King",
      "Viceroy",
      "Grand Duke",
      "Prince",
      "Landgrave",
      "Duke",
      "Marquess",
      "Margrave",
      "Count",  
      "Viscount",
      "Baron", 
      "Baronet",
    };

    {
      char text[100];
      sprintf(text, "Your Score: %d", score);
      font.size(60);
      font.draw(text, glm::vec2(0, 0), ci::ColorA(1, 1, 1, 1));
    }
    {
      char text[100];
      sprintf(text, "Your Rank: %s", ranking_text[ranking]);
      font.size(60);
      font.draw(text, glm::vec2(0, -100), ci::ColorA(1, 1, 1, 1));
    }
  }
#endif


private:
  std::unique_ptr<UI::Canvas> createCanvas(const std::string& camera, const std::string& widgets) noexcept
  {
    return std::make_unique<UI::Canvas>(event_, drawer_,
                                        params_[camera],
                                        Params::load(params_.getValueForKey<std::string>(widgets)));
  }


  // FIXME 変数を後半に定義する実験
  Event<Arguments>& event_;
  ConnectionHolder holder_;

  const ci::JsonTree& params_;

  std::vector<Panel> panels;
  std::unique_ptr<Game> game;

  glm::vec2 camera_center; 

  glm::vec2 left_click_pos;
  bool mouse_draged = false;

  // TIPS:キー入力の判定に集合を利用
  std::set<int> pressing_key;
  
  enum {
    TITLE,
    GAMESTART,        // 開始演出
    GAMEMAIN,
    GAMEEND,          // 終了演出
    RESULT,
  };
  int playing_mode = TITLE;

  // 汎用カウンタ
  Counter counter;

  glm::vec3 cursor_pos; 
  glm::ivec2 field_pos;
  bool can_put = false;

  // 手持ちパネル演出用
  float rotate_offset = 0.0f;
  float hight_offset  = 0.0f;

  // アプリ起動から画面を更新した回数
  u_int frame_counter = 0;

  // プレイ時間計測用
  ci::Timer game_timer;
  double last_time = 0.0;

  // 表示用スコア
  std::vector<int> game_score;
  std::vector<int> game_score_effect;
  

  // カメラ関連
  glm::vec2 rotation;
  float distance;

  glm::vec3 eye_position;
  glm::vec3 target_position;

  Camera field_camera;

  // 表示
  View view;

  // UI関連
  UI::Drawer drawer_;
  std::unique_ptr<UI::Canvas> canvas_;


#ifdef DEBUG
  bool disp_debug_info = false;
#endif
};

}
