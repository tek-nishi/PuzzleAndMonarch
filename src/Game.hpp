#pragma once

//
// ゲーム本編
//

#include <random>
#include <boost/noncopyable.hpp>
#include <cinder/Rand.h>
#include "Logic.hpp"


namespace ngs {

struct Game
  : private boost::noncopyable
{
  Game(const ci::JsonTree& params, Event<Arguments>& event,
       const std::vector<Panel>& panels_) noexcept
    : params_(params),
      event_(event),
      panels(panels_),
      scores(7, 0)
  {
    DOUT << "Panel: " << panels.size() << std::endl;

    // パネルを通し番号で用意
    for (int i = 0; i < panels.size(); ++i)
    {
      waiting_panels.push_back(i);
    }
  }

  ~Game() = default;


  // 内部時間を進める
  void update(double delta_time) noexcept
  {
    if (isPlaying()
#ifdef DEBUG
        && time_count
#endif
        )
    {
      // UI更新
      play_time -= delta_time;

      {
        Arguments args = {
          { "remaining_time", std::max(play_time, 0.0) }
        };
        event_.signal("Game:UI", args);
      }

      if (play_time < 0.0) 
      {
        // 時間切れ
        DOUT << "Time Up." << std::endl;
        endPlay();

        Arguments args = {
          { "scores", scores },
          { "total_score", total_score },
          { "total_ranking", total_ranking },
        };
        event_.signal("Game:Finish", args);
      }
    }
  }


  // 本編準備
  void preparationPlay() noexcept
  {
    // 開始パネルを探す
    std::vector<int> start_panels;
    for (int i = 0; i < panels.size(); ++i)
    {
      if (panels[i].getAttribute() & Panel::START)
      {
        start_panels.push_back(i);
      }
    }
    assert(!start_panels.empty());

    // 乱数を用意
    std::random_device seed_gen;
    std::mt19937 engine(seed_gen());

    if (start_panels.size() > 1)
    {
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
    putPanel(start_panels[0], { 0, 0 }, ci::randInt(4));

    // 次のパネルを決めて、置ける場所も探す
    getNextPanel();
    fieldUpdate();
  }

  // 本編開始
  void beginPlay() noexcept
  {
    play_time = params_.getValueForKey<double>("play_time");
    started = true;

    // UI更新
    Arguments args = {
      { "remaining_time", std::max(play_time, 0.0) }
    };
    event_.signal("Game:UI", args);
    DOUT << "Game started." << std::endl;
  }

  // 本編終了
  void endPlay() noexcept
  {
    finished = true;
    calcTotalScore();
    DOUT << "Game ended." << std::endl;
  }

  // 始まって、結果発表直前まで
  bool isPlaying() const noexcept
  {
    return started && !finished;
  }


  // パネルが置けるか調べる
  bool canPutToBlank(const glm::ivec2& field_pos) const noexcept
  {
    bool can_put = false;
    
    if (std::find(std::begin(blank), std::end(blank), field_pos) != std::end(blank))
    {
      can_put = canPutPanel(panels[hand_panel], field_pos, hand_rotation,
                            field, panels);
    }

    return can_put;
  }

  // そこにblankがあるか？
  bool isBlank(const glm::ivec2& field_pos) const noexcept
  {
    return std::find(std::begin(blank), std::end(blank), field_pos) != std::end(blank);
  }


  // 操作
  void putHandPanel(const glm::ivec2& field_pos) noexcept
  {
    // プレイ中でなければ置けない
    if (!isPlaying()) return;

    // パネルを追加してイベント送信
    putPanel(hand_panel, field_pos, hand_rotation);

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

    // スコア更新
    if (update_score)
    {
      updateScores();
      
      Arguments args = {
        { "scores", scores }
      };
      event_.signal("Game:UpdateScores", args);
    }

    fieldUpdate();

    // 新しいパネル
    if (!getNextPanel())
    {
      // 全パネルを使い切った
      DOUT << "End of panels." << std::endl;
      endPlay();
    }
  }

#if defined (DEBUG)
  // 強制的に次のカード
  void forceNextHandPanel() noexcept
  {
    if (!getNextPanel())
    {
      // 全パネルを使い切った
      DOUT << "End of panels." << std::endl;
      endPlay();
    }
  }
#endif

  void rotationHandPanel() noexcept
  {
    hand_rotation = (hand_rotation + 1) % 4;
  }


  // 手持ちパネル情報
  u_int getHandPanel() const noexcept
  {
    return hand_panel;
  }

  u_int getHandRotation() const noexcept
  {
    return hand_rotation;
  }

  // 配置可能な場所
  const std::vector<glm::ivec2>& getBlankPositions() const noexcept
  {
    return blank;
  }


#if defined (DEBUG)
  // プレイ結果
  void calcResult() const noexcept
  {
    DOUT << "Forest: " << completed_forests.size() << '\n'
         << "  area: " << countTotalAttribute(completed_forests, field, panels) << '\n'
         << "  deep: " << std::count(std::begin(deep_forest), std::end(deep_forest), 1) << '\n'
         << "  Path: " << completed_path.size() << '\n'
         << "length: " << countTotalAttribute(completed_path, field, panels) << '\n'
         << "  Town: " << countTown(completed_path, field, panels)
         << std::endl;
  }

  // 時間の進みON/OFF
  void pauseTimeCount() noexcept
  {
    time_count = !time_count;
    DOUT << "time " << (time_count ? "counted." : "paused.") << std::endl;
  }

  // 手持ちのパネルを変更
  void changePanelForced(int next) noexcept
  {
    hand_panel += next;
    if (hand_panel < 0)
    {
      hand_panel = int(panels.size()) - 1;
    }
    else if (hand_panel >= panels.size())
    {
      hand_panel = 0;
    }
    DOUT << "hand: " << hand_panel << std::endl; 
  }

  // 指定位置周囲のパネルを取得
  std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> enumerateAroundPanels(glm::ivec2 pos) const noexcept
  {
    // 時計回りに調べる
    const glm::ivec2 offsets[] = {
      {  0,  1 },
      {  1,  0 },
      {  0, -1 },
      { -1,  0 },
    };

    std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> around_panels;
    for (const auto& ofs : offsets)
    {
      auto p = pos + ofs;
      if (!field.existsPanel(p)) continue;

      const auto& panel_status = field.getPanelStatus(p);
      around_panels.emplace(p, panel_status);
    }

    return around_panels;
  }
#endif


private:
  bool getNextPanel() noexcept
  {
    if (waiting_panels.empty()) return false;

    hand_panel    = waiting_panels[0];
    hand_rotation = 0;

    waiting_panels.erase(std::begin(waiting_panels));

    DOUT << "Next panel: " << hand_panel << std::endl;

    return true;
  }

  // 各種情報を収集
  void fieldUpdate() noexcept
  {
    blank = field.searchBlank();
  }

  // スコア更新
  void updateScores() noexcept
  {
    scores[0] = int(completed_path.size());
    scores[1] = countTotalAttribute(completed_path, field, panels);
    scores[2] = int(completed_forests.size());
    scores[3] = countTotalAttribute(completed_forests, field, panels);
    scores[4] = int(std::count(std::begin(deep_forest), std::end(deep_forest), 1));
    scores[5] = countTown(completed_path, field, panels);
    scores[6] = int(completed_church.size());
  }

  // 最終スコア
  void calcTotalScore() noexcept
  {
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
  void calcRanking(int score) noexcept
  {
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
    for (total_ranking = 0; total_ranking < 12; ++total_ranking)
    {
      if (ranking_score[total_ranking] <= score) break;
    }
  }

  // パネルを追加してイベント送信
  void putPanel(int panel, const glm::ivec2& pos, u_int rotation) noexcept
  {
    field.addPanel(panel, pos, rotation);
    {
      Arguments args = {
        { "panel",     panel },
        { "field_pos", pos },
        { "rotation",  rotation },
      };
      event_.signal("Game:PutPanel", args);
    }
  }


  // FIXME 参照で持つのいくない
  const ci::JsonTree& params_;
  Event<Arguments>& event_;
  const std::vector<Panel>& panels;

  bool started  = false;
  bool finished = false;

  double play_time = 0;
#ifdef DEBUG
  bool time_count = true;
#endif

  // 配置するパネル
  std::vector<int> waiting_panels;
  int hand_panel;
  u_int hand_rotation;

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
  int total_score   = 0;
  int total_ranking = 0;

  // 列挙した置ける箇所
  std::vector<glm::ivec2> blank;
};

}
