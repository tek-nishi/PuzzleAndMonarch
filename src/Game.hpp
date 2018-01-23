#pragma once

//
// ゲーム本編
//

#include <random>
#include <cinder/Rand.h>
#include "Logic.hpp"


namespace ngs {

struct Game {
  Game(const std::vector<Panel>& panels_)
    : panels(panels_),
      scores(8, 0)
  {
    DOUT << "Panel: " << panels.size() << std::endl;

    // パネルを通し番号で用意
    for (int i = 0; i < panels.size(); ++i) {
      waiting_panels.push_back(i);
    }

    // 開始パネルを探す
    std::vector<int> start_panels;
    for (int i = 0; i < panels.size(); ++i) {
      if (panels[i].getAttribute() & Panel::START) {
        start_panels.push_back(i);
      }
    }
    assert(!start_panels.empty());

    // 乱数を用意
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());

    if (start_panels.size() > 1) {
      // お城が何枚かある時はシャッフル
      std::shuffle(std::begin(start_panels), std::end(start_panels), engine);
    }

    {
      // 最初に置くパネルを取り除いてからシャッフル
      auto it = std::find(std::begin(waiting_panels), std::end(waiting_panels), start_panels[0]);
      if (it != std::end(waiting_panels)) waiting_panels.erase(it);

      std::shuffle(std::begin(waiting_panels), std::end(waiting_panels), engine);
    }
    
    // 最初のパネルを設置
    field.addPanel(start_panels[0], { 0, 0 }, ci::randInt(4));
    field_panels = field.enumeratePanels();
  }

  // 内部時間を進める
  void update(double delta_time) {
    if (isPlaying()
#ifdef DEBUG
        && time_count
#endif
        && ((play_time -= delta_time) < 0.0)) {
      // 時間切れ
      DOUT << "Time Up." << std::endl;
      endPlay();
    }
  }


  // 本編開始
  void beginPlay() {
    started = true;
    // とりあえず３分
    play_time = 60 * 3;

    getNextPanel();
    fieldUpdate();

    DOUT << "Game started." << std::endl;
  }

  // 本編終了
  void endPlay() {
    finished = true;
    calcTotalScore();
    DOUT << "Game ended." << std::endl;
  }

  
  // 残り時間
  double getRemainingTime() const {
    return std::max(play_time, 0.0);
  }


  // プレイ中？
  // 始まったらtrueになり、終了しても変化しない
  bool isBeganPlay() const {
    return started;
  }

  // 始まって、結果発表直前まで
  bool isPlaying() const {
    return started && !finished;
  }

  // 結果発表以降
  bool isEndedPlay() const {
    return finished;
  }


  // パネルが置けるか調べる
  bool canPutToBlank(glm::ivec2 field_pos) {
    bool can_put = false;
    
    if (std::find(std::begin(blank), std::end(blank), field_pos) != std::end(blank)) {
      can_put = canPutPanel(panels[hand_panel], field_pos, hand_rotation,
                            field, panels);
    }

    return can_put;
  }

  // そこにblankがあるか？
  bool isBlank(const glm::ivec2& field_pos) noexcept
  {
    return std::find(std::begin(blank), std::end(blank), field_pos) != std::end(blank);
  }


  // 操作
  void putHandPanel(glm::ivec2 field_pos) {
    // プレイ中でなければ置けない
    if (!isPlaying()) return;

    field.addPanel(hand_panel, field_pos, hand_rotation);

    bool update_score = false;
    {
      // 森完成チェック
      auto completed = isCompleteAttribute(Panel::FOREST, field_pos, field, panels);
      if (!completed.empty()) {
        // 得点
        DOUT << "Forest: " << completed.size() << '\n';
        u_int deep_num = 0;
        for (const auto& comp : completed) {
          DOUT << " Point: " << comp.size() << '\n';

          // 深い森
          bool deep = isDeepForest(comp, field, panels);
          if (deep) {
            deep_num += 1;
          }
          deep_forest.push_back(deep ? 1 : 0);
        }
        DOUT << "  Deep: " << deep_num 
             << std::endl;

        // TIPS コンテナ同士の連結
        std::copy(std::begin(completed), std::end(completed), std::back_inserter(completed_forests));
        update_score = true;
      }
    }
    {
      // 道完成チェック
      auto completed = isCompleteAttribute(Panel::PATH, field_pos, field, panels);
      if (!completed.empty()) {
        // 得点
        DOUT << "  Path: " << completed.size() << '\n';
        for (const auto& comp : completed) {
          DOUT << " Point: " << comp.size() << '\n';
        }
        DOUT << std::endl;

        // TIPS コンテナ同士の連結
        std::copy(std::begin(completed), std::end(completed), std::back_inserter(completed_path));
        update_score = true;
      }
    }
    {
      // 教会完成チェック
      auto completed = isCompleteChurch(field_pos, field, panels);
      if (!completed.empty()) {
        // 得点
        DOUT << "Church: " << completed.size() << std::endl;
              
        // TIPS コンテナ同士の連結
        std::copy(std::begin(completed), std::end(completed), std::back_inserter(completed_church));
        update_score = true;
      }
    }

    if (update_score) {
      updateScores();
    }


    fieldUpdate();

    // 新しいパネル
    if (!getNextPanel()) {
      // 全パネルを使い切った
      DOUT << "End of panels." << std::endl;
      endPlay();
      return;
    }

#if 0
    // パネルをどこかに置けるか調べる
    if (!canPanelPutField(panels[hand_panel], blank,
                          field, panels)) {
      // 置けない…
      DOUT << "Can't put panel." << std::endl;
      endPlay();
      return;
    }
#endif
  }

  // 強制的に次のカード
  void forceNextHandPanel() {
    if (!getNextPanel()) {
      // 全パネルを使い切った
      DOUT << "End of panels." << std::endl;
      endPlay();
      return;
    }
  }

  void rotationHandPanel() {
    hand_rotation = (hand_rotation + 1) % 4;
  }


  // 手持ちパネル情報
  u_int getHandPanel() const {
    return hand_panel;
  }

  u_int getHandRotation() const {
    return hand_rotation;
  }

  // フィールド情報
  const std::vector<PanelStatus>& getFieldPanels() const {
    return field_panels;
  }

  const std::vector<glm::ivec2>& getBlankPositions() const {
    return blank;
  };

  glm::vec2 getFieldCenter() const {
    return field_center;
  }


  // プレイ中のスコア
  const std::vector<int>& getScores() const {
    return scores;
  }

  // 最終スコア
  int getTotalScore() const {
    return total_score;
  }

  int getTotalRanking() const {
    return total_ranking;
  }


  // プレイ結果
  void calcResult() {
    DOUT << "Forest: " << completed_forests.size() << '\n'
         << "  area: " << countTotalAttribute(completed_forests, field, panels) << '\n'
         << "  deep: " << std::count(std::begin(deep_forest), std::end(deep_forest), 1) << '\n'
         << "  Path: " << completed_path.size() << '\n'
         << "length: " << countTotalAttribute(completed_path, field, panels) << '\n'
         << "  Town: " << countTown(completed_path, field, panels)
         << std::endl;
  }

#ifdef DEBUG
  // 時間の進みON/OFF
  void pauseTimeCount() {
    time_count = !time_count;
    DOUT << "time " << (time_count ? "counted." : "paused.") << std::endl;
  }

  // 手持ちのパネルを変更
  void changePanelForced(int next) {
    hand_panel += next;
    if (hand_panel < 0) {
      hand_panel = int(panels.size()) - 1;
    }
    else if (hand_panel >= panels.size()) {
      hand_panel = 0;
    }
    DOUT << "hand: " << hand_panel << std::endl; 
  }

  // 指定位置周囲のパネルを取得
  std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> enumerateAroundPanels(glm::ivec2 pos) const {
    // 時計回りに調べる
    const glm::ivec2 offsets[] = {
      {  0,  1 },
      {  1,  0 },
      {  0, -1 },
      { -1,  0 },
    };

    std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> around_panels;
    for (const auto& ofs : offsets) {
      auto p = pos + ofs;
      if (!field.existsPanel(p)) continue;

      const auto& panel_status = field.getPanelStatus(p);
      around_panels.insert({ p, panel_status });
    }

    return around_panels;
  }

#endif


private:
  // FIXME 参照で持つのいくない
  const std::vector<Panel>& panels;

  bool started    = false;
  bool finished   = false;

  double play_time = 0;
#ifdef DEBUG
  bool time_count = true;
#endif

  std::vector<int> waiting_panels;
  int hand_panel;
  u_int hand_rotation;
  bool check_all_blank;

  Field field;

  // 完成した森
  std::vector<std::vector<glm::ivec2>> completed_forests;
  // 深い森
  std::vector<u_int> deep_forest;
  // 完成した道
  std::vector<std::vector<glm::ivec2>> completed_path;
  // 完成した教会
  std::vector<glm::ivec2> completed_church;
  // スコア
  std::vector<int> scores;
  int total_score = 0;
  int total_ranking = 0;

  // 列挙したフィールド上のパネル
  std::vector<PanelStatus> field_panels;
  // 列挙した置ける箇所
  std::vector<glm::ivec2> blank;
  // なんとなく中心位置
  glm::vec2 field_center;


  bool getNextPanel() {
    if (waiting_panels.empty()) return false;

    hand_panel      = waiting_panels[0];
    hand_rotation   = 0;
    check_all_blank = true;

    waiting_panels.erase(std::begin(waiting_panels));

    DOUT << "Next panel: " << hand_panel << std::endl;

    return true;
  }

  // 各種情報を収集
  void fieldUpdate() {
    field_panels = field.enumeratePanels();
    blank        = field.searchBlank();

    // 中心座標をなんとなく計算
    glm::vec2 center;
    for (auto p  : field_panels) {
      center += p.position;
    }
    center /= float(field_panels.size());
    field_center = center; 
  }

  // スコア更新
  void updateScores() {
    scores[0] = int(completed_path.size());
    scores[1] = countTotalAttribute(completed_path, field, panels);
    scores[2] = int(completed_forests.size());
    scores[3] = countTotalAttribute(completed_forests, field, panels);
    scores[4] = int(std::count(std::begin(deep_forest), std::end(deep_forest), 1));
    scores[5] = countTown(completed_path, field, panels);
    scores[6] = int(completed_church.size());
  }

  // 最終スコア
  void calcTotalScore() {
    // FIXME 適当に加算しているだけ…
    total_score = scores[0] * 3            // 完成した道の数
                  + scores[1] * 4          // 道の長さ
                  + scores[2] * 5          // 完成した森の数
                  + scores[3] * 8          // 森の面積
                  + scores[4] * 15         // 深い森の数
                  + scores[5] * 4          // 街の数
                  + scores[6] * 20;        // 教会の数

    total_score *= 50;

    calcRanking(total_score);
  }

  // ランキングを決める
  void calcRanking(int score) {
    const int ranking_score[] = {
      12 * 12 * 700,
      11 * 11 * 700,
      10 * 10 * 700,
      9 * 9 * 700,
      8 * 8 * 700,
      7 * 7 * 700,
      6 * 6 * 700,
      5 * 5 * 700,
      4 * 4 * 700,
      3 * 3 * 700,
      2 * 2 * 700,
      1 * 1 * 700,
      0 * 0 * 700,
    };
    for (total_ranking = 0; total_ranking < 12; ++total_ranking) {
      if (ranking_score[total_ranking] <= score) break;
    }
  }
};

}
