#pragma once

//
// アプリ本編
//

#include "Defines.hpp"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <cinder/Rand.h>
#include <cinder/Camera.h>
#include <cinder/CameraUi.h>
#include <cinder/Ray.h>
#include <cinder/Timer.h>
#include "Params.hpp"
#include "JsonUtil.hpp"
#include "Game.hpp"
#include "Counter.hpp"
#include "Font.hpp"
#include "View.hpp"
#include "Shader.hpp"
#include "UIWidgetsFactory.hpp"


namespace ngs {

class MainPart {

public:
  MainPart(ci::JsonTree& params)
    : panels(createPanels()),
      game(std::make_unique<Game>(panels)),
      fov(params.getValueForKey<float>("field_camera.fov")),
      near_z(params.getValueForKey<float>("field_camera.near_z")),
      far_z(params.getValueForKey<float>("field_camera.far_z")),
      distance(params.getValueForKey<float>("field_camera.distance")),
      field_camera(ci::app::getWindowWidth(), ci::app::getWindowHeight(), fov, near_z, far_z),
      camera_ui(&field_camera),
      view(createView()),
      ui_fov(params.getValueForKey<float>("ui.camera.fov")),
      ui_near_z(params.getValueForKey<float>("ui.camera.near_z")),
      ui_far_z(params.getValueForKey<float>("ui.camera.far_z")),
      ui_camera(ci::app::getWindowWidth(), ci::app::getWindowHeight(), ui_fov, ui_near_z, ui_far_z),
      widgets_(widgets_factory_.construct(params["title.widgets"])),
      drawer_(params)
  {
    // フィールドカメラ
    glm::quat q = Json::getQuat(params["field_camera.rotation"]);
    glm::vec3 p = q * glm::vec3{ 0, 0, -distance };
    field_camera.lookAt(p, glm::vec3(0));

    // UI関連
    ui_camera.lookAt(Json::getVec<glm::vec3>(params["ui.camera.eye"]), Json::getVec<glm::vec3>(params["ui.camera.target"]));
  }


  void mouseMove(ci::app::MouseEvent event) {
    auto pos = event.getPos();
    calcFieldPos(pos);
  }

	void mouseDown(ci::app::MouseEvent event) {
    if (!event.isLeft()) return;

    mouse_draged   = false;
    left_click_pos = event.getPos();
    

    camera_ui.mouseDown(event);
  }
  
	void mouseDrag(ci::app::MouseEvent event) {
    if (!event.isLeftDown()) return;

    // クリック位置から3pixel程度の動きは無視
    glm::vec2 pos = event.getPos();
    if (glm::distance2(left_click_pos, pos) < 9.0f) return;

    mouse_draged = true;
    camera_ui.mouseDrag(event);
  }
  
	void mouseUp(ci::app::MouseEvent event) {
    camera_ui.mouseUp(event);

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
  }

  void mouseWheel(ci::app::MouseEvent event) {
    camera_ui.mouseWheel(event);
  }
  
  void keyDown(ci::app::KeyEvent event) {
    int code = event.getCode();
    pressing_key.insert(code);

#ifdef DEBUG
    if (code == ci::app::KeyEvent::KEY_r) {
      // 強制リセット
      game = std::make_unique<ngs::Game>(panels);
      playing_mode = TITLE;
    }
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

  void keyUp(ci::app::KeyEvent event) {
    int code = event.getCode();
    pressing_key.erase(code);
  }
  

	void update() {
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


  void resize(const float aspect) {
    setupCamera(field_camera, aspect, fov);
    setupCamera(ui_camera, aspect, ui_fov);

    DOUT << "resize: " << ci::app::getWindowSize() << std::endl;
  }


	void draw(glm::ivec2 window_size) {
    ci::gl::clear(ci::Color(0, 0, 0));
    
    // 本編
    ci::gl::disableAlphaBlending();

    // プレイ画面
    ci::gl::setMatrices(field_camera);
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
    ci::gl::enableDepth(false);
    ci::gl::disable(GL_CULL_FACE);
    ci::gl::enableAlphaBlending();
    ci::gl::setMatrices(ui_camera);

    {
      glm::vec3 top_left;
      glm::vec3 top_right;
      glm::vec3 bottom_left;
      glm::vec3 bottom_right;
      ui_camera.getNearClipCoordinates(&top_left, &top_right, &bottom_left, &bottom_right);
      ci::Rectf rect(top_left.x, top_left.y, bottom_right.x, bottom_right.y);
      widgets_->draw(rect, glm::vec2(1, 1), drawer_);
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
    float x = pos.x / float(ci::app::getWindowWidth());
    float y = 1.0f - pos.y / float(ci::app::getWindowHeight());

    // 画面奥に伸びるRayを生成
    ci::Ray ray = field_camera.generateRay(x, y,
                                           field_camera.getAspectRatio());

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
  // FIXME 変数を後半に定義する実験
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


  float fov;
  float near_z;
  float far_z;
  float distance;

  ci::CameraPersp field_camera;
  ci::CameraUi camera_ui;
  
  View view;


  // UI関連
  float ui_fov;
  float ui_near_z;
  float ui_far_z;

  ci::CameraPersp ui_camera;

  UI::WidgetPtr widgets_; 
  UI::WidgetsFactory widgets_factory_;

  UI::Drawer drawer_;


#ifdef DEBUG
  bool disp_debug_info = false;
#endif
};

}
