//
// 見習い君主
//

#include "Defines.hpp"
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>
#include <cinder/Camera.h>
#include <cinder/CameraUi.h>
#include <cinder/Sphere.h>
#include <cinder/Ray.h>
#include "Game.hpp"
#include "Counter.hpp"
#include "Font.hpp"
#include "View.hpp"
#include "Shader.hpp"


using namespace ci;
using namespace ci::app;


namespace ngs {

class MyApp : public App {

public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp()
    : panels(createPanels()),
      game(std::make_unique<Game>(panels)),
      camera_ui(&field_camera),
      view(createView()),
      font("MAIAN.TTF"),
      jpn_font("DFKAIC001.ttf")
  {
    Rand::randomize();

    // フィールドカメラ
    field_camera = CameraPersp(getWindowWidth(), getWindowHeight(),
                               fov,
                               near_z, far_z);
    quat q(toRadians(vec3{ 30, 45, 0 }));
    vec3 p = q * vec3{ 0, 0, -distance };
    field_camera.lookAt(p, vec3(0));

    // UIカメラ
    auto half_size = getWindowSize() / 2;
    ui_camera = CameraOrtho(-half_size.x, half_size.x,
                            -half_size.y, half_size.y,
                            -1, 1);
    ui_camera.lookAt(vec3(0), vec3(0, 0, -1));
    
    {
      // 残り時間表示のx位置はあらかじめ決めておく
      font.size(80);
      auto size = font.drawSize("9'99");
      remain_time_x = -size.x / 2;
    }

    shader_font = createShader("font", "font");

#if defined (CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    getSignalSupportedOrientations().connect([]() {
        return ci::app::InterfaceOrientation::All;
      });
#endif
  }


private:
  void mouseMove(MouseEvent event) override {
    auto pos = event.getPos();
    calcFieldPos(pos);
  }

	void mouseDown(MouseEvent event) override {
    if (!event.isLeft()) return;

    mouse_draged   = false;
    left_click_pos = event.getPos();
    

    camera_ui.mouseDown(event);
  }
  
	void mouseDrag(MouseEvent event) override {
    if (!event.isLeftDown()) return;

    // クリック位置から3pixel程度の動きは無視
    ci::vec2 pos = event.getPos();
    if (glm::distance2(left_click_pos, pos) < 9.0f) return;

    mouse_draged = true;
    camera_ui.mouseDrag(event);
  }
  
	void mouseUp(MouseEvent event) override {
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

  void mouseWheel(MouseEvent event) override {
    camera_ui.mouseWheel(event);
  }
  
  void keyDown(KeyEvent event) override {
    int code = event.getCode();
    pressing_key.insert(code);

#ifdef DEBUG
    if (code == KeyEvent::KEY_r) {
      // 強制リセット
      game = std::make_unique<ngs::Game>(panels);
      playing_mode = TITLE;
    }
    if (code == KeyEvent::KEY_t) {
      // 時間停止
      game->pauseTimeCount();
    }
    if (code == KeyEvent::KEY_e) {
      // 強制終了
      game->endPlay();
    }

    if (code == KeyEvent::KEY_d) {
      disp_debug_info = !disp_debug_info;
    }

    // 手持ちパネル強制変更
    if (code == KeyEvent::KEY_b) {
      game->changePanelForced(-1);
    }
    else if (code == KeyEvent::KEY_n) {
      game->changePanelForced(1);
    }
#endif

    switch (playing_mode) {
    case GAMEMAIN:
      if (game->isPlaying()) {
        // 強制的に次のカードを引く
        if (code == KeyEvent::KEY_f) {
          game->forceNextHandPanel();
          rotate_offset = 0.0f;
          hight_offset  = 500.0f;
          can_put = false;
        }
      }
      break;
    }
  }

  void keyUp(KeyEvent event) override {
    int code = event.getCode();
    pressing_key.erase(code);
  }
  

	void update() override {
    counter.update();
    game->update();

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
          DOUT << "GAMEMAIN." << std::endl;
        }
      }
      break;

    case GAMEMAIN:
      if (!game->isPlaying()) {
        // 結果画面へ
        playing_mode = GAMEEND;
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


  void resize() override {
    float aspect = getWindowAspectRatio();
    field_camera.setAspectRatio(aspect);
    if (aspect < 1.0) {
      // 画面が縦長になったら、幅基準でfovを求める
      // fovとnear_zから投影面の幅の半分を求める
      float half_w = std::tan(toRadians(fov / 2)) * near_z;

      // 表示画面の縦横比から、投影面の高さの半分を求める
      float half_h = half_w / aspect;

      // 投影面の高さの半分とnear_zから、fovが求まる
      float fov_w = std::atan(half_h / near_z) * 2;
      field_camera.setFov(toDegrees(fov_w));
    }
    else {
      // 横長の場合、fovは固定
      field_camera.setFov(fov);
    }

    auto half_size = getWindowSize() / 2;
    ui_camera = CameraOrtho(-half_size.x, half_size.x,
                            -half_size.y, half_size.y,
                            -1, 1);
    ui_camera.lookAt(vec3(0), vec3(0, 0, -1));

    DOUT << "resize: " << half_size << std::endl;
  }


	void draw() override {
    gl::clear(Color(0, 0, 0));
    
    // 本編
    gl::disableAlphaBlending();

    // プレイ画面
    gl::setMatrices(field_camera);
    gl::translate(-camera_center.x, -5.0, -camera_center.y);

    gl::enableDepth();
    gl::enable(GL_CULL_FACE);

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
            glm::ivec2 pos = it.first;
            pos *= int(ngs::PANEL_SIZE);

            glm::vec3 p(pos.x, 0.0f, pos.y);

            auto status = it.second;
            ngs::drawPanelEdge(panels[status.number], p, status.rotation);
          }
        }
      }
#endif
    }
    drawFieldBg(view);

    // UI
    gl::enableDepth(false);
    gl::disable(GL_CULL_FACE);
    gl::enableAlphaBlending();

    gl::setMatrices(ui_camera);

    {
      gl::ScopedGlslProg prog(shader_font);

      switch (playing_mode) {
      case TITLE:
        {
          {
            font.size(100);
            std::string text("Apprentice Monarch");
            auto size = font.drawSize(text);
            font.draw(text, vec2(-size.x / 2, 150), ColorA(1, 1, 1, 1));
          }
          {
            font.size(60);
            std::string text("Left click to start");
            auto size = font.drawSize(text);

            float r = frame_counter * 0.05f;
            font.draw(text, vec2(-size.x / 2, -300), ColorA(1, 1, 1, (std::sin(r) + 1.0f) * 0.5f));
          }
        }
        break;

      case GAMESTART:
        {
          font.size(90);
          const char* text = "Secure the territory!";

          auto size = font.drawSize(text);
          font.draw(text, vec2(-size.x / 2.0f, -size.y / 2.0f), ColorA(1, 1, 1, 1));
        }
        break;

      case GAMEMAIN:
        {
          // 残り時間
          font.size(80);

          char text[100];
          u_int remainig_time = (game->getRemainingTime() + 59) / 60;
          u_int minutes = remainig_time / 60;
          u_int seconds = remainig_time % 60;
          sprintf(text, "%d'%02d", minutes, seconds);

          // 時間が10秒切ったら色を変える
          ColorA color(1, 1, 1, 1);
          if (remainig_time <= 10) {
            color = ColorA(1, 0, 0, 1);
          }

          int hh = getWindowHeight() / 2;

          font.draw(text, vec2(remain_time_x, hh - 100), color);
        }
        {
          int hw = getWindowWidth() / 2;
          drawGameInfo(25, vec2(-hw + 50, 0), -40);
        }
        break;

      case GAMEEND:
        {
          font.size(90);
          const char* text = "Well done!";

          auto size = font.drawSize(text);
          font.draw(text, vec2(-size.x / 2.0f, -size.y / 2.0f), ColorA(1, 1, 1, 1));
        }
        break;

      case RESULT:
        {
          {
            font.size(80);
            std::string text("Result");
            auto size = font.drawSize(text);
            font.draw(text, vec2(-size.x / 2, 250), ColorA(1, 1, 1, 1));
          }
          {
            font.size(40);
            std::string text("Left click to title");
            auto size = font.drawSize(text);

            int hh = getWindowHeight() / 2;
            float r = frame_counter * 0.05f;
            font.draw(text, vec2(-size.x / 2, -hh + 50), ColorA(1, 1, 1, (std::sin(r) + 1.0f) * 0.5f));
          }
          drawResult();
        }
        break;
      }
    }
  }


  void calcFieldPos(vec2 pos) {
    float x = pos.x / float(getWindowWidth());
    float y = 1.0f - pos.y / float(getWindowHeight());

    // 画面奥に伸びるRayを生成
    Ray ray = field_camera.generateRay(x, y,
                                       field_camera.getAspectRatio());

    auto m = glm::translate(vec3{ camera_center.x, 5, camera_center.y });
    auto origin = m * vec4(ray.getOrigin(), 1);
    ray.setOrigin(origin);

    // 地面との交差を調べ、正確な位置を計算
    float z;
    float on_field = ray.calcPlaneIntersection(vec3(0), vec3(0, 1, 0), &z);
    can_put = false;
    if (on_field) {
      cursor_pos = ray.calcPosition(z);
      
      field_pos.x = roundValue(cursor_pos.x, PANEL_SIZE);
      field_pos.y = roundValue(cursor_pos.z, PANEL_SIZE);
      can_put = game->canPutToBlank(field_pos);

      // 多少違和感を軽減するために位置を再計算
      ray.calcPlaneIntersection(vec3(0, 10, 0), vec3(0, 1, 0), &z);
      cursor_pos = ray.calcPosition(z);
    }
  }

  // プレイ情報を表示
  void drawGameInfo(int font_size, vec2 pos, float next_y) {
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
      ColorA col = game_score_effect[i] ? ColorA(1, 0, 0, 1)
                                        : ColorA(1, 1, 1, 1);

      char buffer[100];
      sprintf(buffer, t, game_score[i]);
      jpn_font.draw(buffer, pos,col);

      pos.y += next_y;
      i += 1;
    }
  }

  // 結果を表示
  void drawResult() {
    drawGameInfo(30, vec2(-300, 150), -50);

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
      font.draw(text, vec2(0, 0), ColorA(1, 1, 1, 1));
    }
    {
      char text[100];
      sprintf(text, "Your Rank: %s", ranking_text[ranking]);
      font.size(60);
      font.draw(text, vec2(0, -100), ColorA(1, 1, 1, 1));
    }
  }




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

  vec3 cursor_pos; 
  ivec2 field_pos;
  bool can_put = false;

  // 手持ちパネル演出用
  float rotate_offset = 0.0f;
  float hight_offset  = 0.0f;

  // 残り時間表示位置(xのみ)
  int remain_time_x;

  // アプリ起動から画面を更新した回数
  u_int frame_counter = 0;

  // 表示用スコア
  std::vector<int> game_score;
  std::vector<int> game_score_effect;


  float fov      = 25.0f;
  float near_z   = 1.0f;
  float far_z    = 1000.0f;
  float distance = 160.0f;

  CameraPersp field_camera;
  CameraOrtho ui_camera;

  CameraUi camera_ui;

  View view;

  Font font;
  Font jpn_font;

  ci::gl::GlslProgRef shader_font;



#ifdef DEBUG
  bool disp_debug_info = false;
#endif
};

}


CINDER_APP(ngs::MyApp, RendererGl,
           [](App::Settings* settings) {
             settings->setWindowSize(ivec2(960, 640));
             settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));
             
             settings->setPowerManagementEnabled(true);
             settings->setHighDensityDisplayEnabled(false);
           });
