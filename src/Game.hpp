#pragma once

//
// ゲーム本編
//

#include <random>
#include <numeric>
#include <boost/noncopyable.hpp>
#include <cinder/Rand.h>
#include "Logic.hpp"
#include "CountExec.hpp"
#include "TextCodec.hpp"


namespace ngs {

struct Game
  : private boost::noncopyable
{
  Game(const ci::JsonTree& params, Event<Arguments>& event,
       bool purchased,
       const std::vector<Panel>& panels) noexcept
    : params_(params),
      event_(event),
      panels_(panels),
      initial_play_time_(params.getValueForKey<double>("play_time")),
      play_time_(initial_play_time_),
      scores_(7, 0)
  {
    DOUT << "Panel: " << panels_.size() << std::endl;

    // 課金済みなら制限時間を伸ばす
    if (purchased)
    {
      DOUT << "Time extended." << std::endl;
      initial_play_time_ = params.getValueForKey<double>("play_time_extend");
      play_time_         = initial_play_time_;
    }

    // 乱数
    std::random_device seed_gen;
    engine_ = std::mt19937(seed_gen());

    // パネルを通し番号で用意
    waiting_panels.resize(panels_.size());
    std::iota(std::begin(waiting_panels), std::end(waiting_panels), 0);

    // パネルを準備
    preparationPanel();
  }

  ~Game() = default;


  // 内部時間を進める
  void update(double delta_time) noexcept
  {
    count_exec_.update(delta_time);

    if (isPlaying()
#ifdef DEBUG
        && time_count
#endif
        )
    {
      play_time_ -= delta_time;

      updateGameUI();

      if (play_time_ < 0.0) 
      {
        // 時間切れ
        DOUT << "Time Up." << std::endl;
        endPlay();
      }
    }
  }


  // ゲームが始まってから進んだ時間 [0, 1]
  double getPlayTimeRate() const noexcept
  {
    return 1.0 - glm::clamp(play_time_ / initial_play_time_, 0.0, 1.0); 
  }

  double getPlayTime() const noexcept
  {
    return std::max(play_time_, 0.0);
  }

  // 制限時間
  double getLimitTime() const noexcept
  {
    return initial_play_time_;
  }


  // 本編準備
  void putFirstPanel() noexcept
  {
    // 最初のパネルを設置
    putPanel(start_panel_, { 0, 0 }, ci::randInt(4));
  }

  void preparationPlay(bool tutorial) noexcept
  {
    if (tutorial) orderForTutorial();

    // 次のパネルを決めて、置ける場所も探す
    getNextPanel();
  }

  // 本編開始
  void beginPlay() noexcept
  {
    started = true;
    DOUT << "Game started." << std::endl;
  }

  // 本編終了
  void endPlay() noexcept
  {
    finished = true;
    calcResults();

    Arguments args{
      { "scores",        scores_ },
      { "total_score",   total_score },
      { "total_ranking", total_ranking },
      { "total_panels",  total_panels },
      { "perfect",       waiting_panels.empty() },
      
      { "panel_turned_times", panel_turned_times_ },
      { "panel_moved_times",  panel_moved_times_ },

      { "max_forest", max_forest_ },
      { "max_path",   max_path_ },

      { "completed_forest", &completed_forests },
      { "completed_path",   &completed_path },
    };
    event_.signal("Game:Finish", args);
    
    DOUT << "Game ended." << std::endl;
  }

  // 強制終了
  void abortPlay() noexcept
  {
    finished = true;
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
    
    if (std::find(std::begin(blank_), std::end(blank_), field_pos) != std::end(blank_))
    {
      can_put = canPutPanel(panels_[hand_panel], field_pos, hand_rotation, field);
    }

    return can_put;
  }

  // そこにblankがあるか？
  bool isBlank(const glm::ivec2& field_pos) const noexcept
  {
    return std::find(std::begin(blank_), std::end(blank_), field_pos) != std::end(blank_);
  }


#if defined (DEBUG)

  void testPutPanel(const glm::ivec2& field_pos, int panel, u_int rotation)
  {
    putPanel(panel, field_pos, rotation);
    checkFieldStatus(field_pos);
  }

  void testCalcResults()
  {
    calcResults();
    DOUT << "Score: " << total_score << std::endl;
    DOUT << " Rank: " << total_ranking << std::endl;
    
    sendScores();
  }

#endif


  // 操作
  void putHandPanel(const glm::ivec2& field_pos) noexcept
  {
    // プレイ中でなければ置けない
    if (!isPlaying()) return;

    // パネルを追加してイベント送信
    total_panels += 1;
    putPanel(hand_panel, field_pos, hand_rotation);

    // 状況チェック
    checkFieldStatus(field_pos);

    // 新しいパネル
    if (!getNextPanel())
    {
      // 全パネルを使い切った
      DOUT << "End of panels." << std::endl;
      endPlay();
    }
  }

  // 状況チェック
  void checkFieldStatus(const glm::ivec2& field_pos)
  {
    bool update_score = false;
    {
      // 森完成チェック
      auto completed = isCompleteAttribute(Panel::FOREST, field_pos, field, panels_);
      if (!completed.empty())
      {
        // 得点
        DOUT << "Forest: " << completed.size() << '\n';
        u_int deep_num = 0;
        for (const auto& comp : completed)
        {
          // 深い森
          auto deep = countDeepForest(comp, field, panels_);
          deep_num += deep;
          deep_forest.push_back(deep);

          // 最大の大きさを保存
          max_forest_ = std::max(max_forest_, u_int(comp.size()));

          DOUT << " Point: " << comp.size() << '\n';
          DOUT << "  Deep: " << deep << '\n';
        }
        DOUT << "Total Deep: " << deep_num << '\n';
        DOUT << "Max forest: " << max_forest_ << '\n';
        DOUT << std::endl;

        appendContainer(completed, completed_forests);

        Arguments args{
          { "completed", completed }
        };
        event_.signal("Game:completed_forests", args);

        update_score = true;
      }
    }
    {
      // 道完成チェック
      auto completed = isCompleteAttribute(Panel::PATH, field_pos, field, panels_);
      if (!completed.empty())
      {
        // 得点
        DOUT << "  Path: " << completed.size() << '\n';
        for (const auto& comp : completed)
        {
          // 最大の大きさを保存
          max_path_ = std::max(max_path_, u_int(comp.size()));

          DOUT << " Point: " << comp.size() << '\n';
        }
        DOUT << "Max path: " << max_path_ << '\n';
        DOUT << std::endl;

        appendContainer(completed, completed_path);

        Arguments args{
          { "completed", completed }
        };
        event_.signal("Game:completed_path", args);

        update_score = true;
      }
    }
    {
      // 教会完成チェック
      auto completed = isCompleteChurch(field_pos, field, panels_);
      if (!completed.empty())
      {
        // 得点
        DOUT << "Church: " << completed.size() << std::endl;
              
        appendContainer(completed, completed_church);
        
        Arguments args{
          { "completed", completed }
        };
        event_.signal("Game:completed_church", args);

        update_score = true;
      }
    }

    // スコア更新
    if (update_score)
    {
      updateScores();
      
      Arguments args{
        { "scores", scores_ },
      };
      event_.signal("Game:UpdateScores", args);
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
    panel_turned_times_ += 1;
  }

  void moveHandPanel(const glm::vec2& pos) noexcept
  {
    panel_moved_times_ += 1;
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
    return blank_;
  }

  // パネルを置く場所を適当に決める
  glm::ivec2 getNextPanelPosition(const glm::ivec2& put_pos) noexcept
  {
    auto positions = getBlankPositions();
    // 適当に並び替える
    std::shuffle(std::begin(positions), std::end(positions), engine_);

    // 置いた場所から一番距離の近い場所を選ぶ
    auto it = std::min_element(std::begin(positions), std::end(positions),
                               [put_pos](const glm::ivec2& a, const glm::ivec2& b) noexcept
                               {
                                 // FIXME 整数型のベクトルだとdistance2とかdotとかが使えない
                                 auto da = put_pos - a;
                                 auto db = put_pos - b;

                                 return (da.x * da.x + da.y * da.y) < (db.x * db.x + db.y * db.y);
                               });

    return *it;
  }

  // 指定属性のパネルを探す
  std::tuple<bool, glm::ivec2, int> searchAttribute(u_int attribute, u_int edge) const
  {
    uint64_t e = edge;
    uint64_t edge_bundled = e | (e << 16) | (e << 32) | (e << 48);

    const auto& panel_positions = field.getPanelPositions();
    auto it = std::find_if(std::begin(panel_positions), std::end(panel_positions),
                           [this, attribute, edge_bundled](const glm::ivec2& pos)
                           {
                             const auto& status = field.getPanelStatus(pos);
                             const auto& panel  = panels_[status.number];

                             return (panel.getAttribute() & attribute) || (edge_bundled & status.edge);
                           });

    if (it == std::end(panel_positions))
    {
      return { false, glm::ivec2(0), 0 };
    }

    // edge情報をゲット
    int rotate = 0;
    if (edge)
    {
      const auto& status = field.getPanelStatus(*it);
      auto rotated_edge  = status.edge;

      for ( ; rotate < 4; ++rotate)
      {
        if (rotated_edge & e)
        {
          break;
        }
        // NOTICE 事前に定義した変数を変更している
        e <<= 16;
      }
    }

    return { true, *it, rotate };
  }


  // 保存
  void save(const std::string& name) const noexcept
  {
    ci::JsonTree save_data;

    save_data.addChild(ci::JsonTree("hand_panel", hand_panel))
             .addChild(ci::JsonTree("hand_rotation", hand_rotation))
             .addChild(Json::createArray("waiting_panels", waiting_panels))
             .addChild(field.serialize())
             .addChild(ci::JsonTree("play_time", play_time_))
             .addChild(Json::createVecVecArray("completed_forests", completed_forests))
             .addChild(Json::createArray("deep_forest", deep_forest))
             .addChild(Json::createVecVecArray("completed_path", completed_path))
             .addChild(Json::createVecArray("completed_church", completed_church))
             .addChild(ci::JsonTree("panel_turned_times", panel_turned_times_))
             .addChild(ci::JsonTree("panel_moved_times", panel_moved_times_))
             ;


#if defined(OBFUSCATION_GAME_RECORD)
    TextCodec::write((getDocumentPath() / name).string(), save_data.serialize());
#else
    save_data.write(getDocumentPath() / name);
#endif

    DOUT << "Game saved: " << name << std::endl;
  }

  // NOTE pathはfull path
  void load(const ci::fs::path& path)
  {
#if defined (DEBUG)
    game_path = path.string();
#endif

    // auto full_path = getDocumentPath() / path;
    if (!ci::fs::is_regular_file(path))
    {
      DOUT << "No game data." << std::endl;
      return;
    }

    ci::JsonTree json;
#if defined (OBFUSCATION_GAME_RECORD)
    auto text = TextCodec::load(path.string());
    try
    {
      json = ci::JsonTree(text);
    }
    catch (ci::JsonTree::ExcJsonParserError& exc)
    {
      DOUT << "Game record broken." << std::endl;
      return;
    }
#else
    try
    {
      json = ci::JsonTree(ci::loadFile(path));
    }
    catch (ci::JsonTree::ExcJsonParserError& exc)
    {
      DOUT << "Game record broken." << std::endl;
      return;
    }
#endif

    count_exec_.clear();

    hand_panel     = json.getValueForKey<int>("hand_panel");
    hand_rotation  = json.getValueForKey<u_int>("hand_rotation");
    waiting_panels = Json::getArray<int>(json["waiting_panels"]);
    field          = Field(json["field"]);
    play_time_     = json.getValueForKey<double>("play_time");
    
    completed_forests = Json::getVecVecArray<glm::ivec2>(json["completed_forests"]);
    deep_forest       = Json::getArray<u_int>(json["deep_forest"]);
    completed_path    = Json::getVecVecArray<glm::ivec2>(json["completed_path"]);
    completed_church  = Json::getVecArray<glm::ivec2>(json["completed_church"]);

    panel_turned_times_ = json.getValueForKey<u_int>("panel_turned_times");
    panel_moved_times_  = json.getValueForKey<u_int>("panel_moved_times");

    auto panels = field.enumeratePanels();
    double at_time       = params_.getValueForKey<double>("replay.delay");
    double interval_time = params_.getValueForKey<double>("replay.interval");
    for (const auto& status : panels)
    {
      count_exec_.add(at_time,
                      [status, this]() noexcept
                      {
                        Arguments args{
                          { "panel",     status.number },
                          { "field_pos", status.position },
                          { "rotation",  status.rotation },
                        };
                        event_.signal("Game:PutPanel", args);
                      });
      at_time += interval_time;;
    }
    // NOTICE 最初に置かれているパネルは除く
    total_panels = u_int(panels.size()) - 1;

    // スコア情報は時間差で送信
    updateScores();
    calcResults();
    sendScores();
  }


  // Fieldの中心位置と広さを計算
  std::pair<glm::vec3, float> getFieldCenterAndDistance(bool blank = true) const noexcept
  {
    std::vector<glm::vec3> points;
    const auto& positions = blank ? getBlankPositions() : field.getPanelPositions();
    for (const auto& p : positions)
    {
      // Panelの４隅の座標を加える
      points.emplace_back(p.x - 0.5, 0.0f, p.y - 0.5);
      points.emplace_back(p.x + 0.5, 0.0f, p.y - 0.5);
      points.emplace_back(p.x - 0.5, 0.0f, p.y + 0.5);
      points.emplace_back(p.x + 0.5, 0.0f, p.y + 0.5);
    }

    // 登録した点の平均値→注視点
    auto center = std::accumulate(std::begin(points), std::end(points), glm::vec3()) / float(points.size());

    // 中心から一番遠い座標から距離を決める
    auto it = std::max_element(std::begin(points), std::end(points),
                               [center](const glm::vec3& a, const glm::vec3& b) noexcept
                               {
                                 auto d1 = glm::distance2(a, center);
                                 auto d2 = glm::distance2(b, center);
                                 return d1 < d2;
                               });
    auto d = glm::distance(*it, center);

    return { center, d };
  }

  // UI更新
  void updateGameUI() const noexcept
  {
    // UI更新
    Arguments args{
      { "remaining_time", getPlayTime() }
    };
    event_.signal("Game:UI", args);
  }


#if defined (DEBUG)
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
      hand_panel = int(panels_.size()) - 1;
    }
    else if (hand_panel >= panels_.size())
    {
      hand_panel = 0;
    }
    DOUT << "hand: " << hand_panel << std::endl; 
  }

  // 指定位置周囲のパネルを取得
  std::map<glm::ivec2, PanelStatus, LessVec<glm::ivec2>> enumerateAroundPanels(const glm::ivec2& pos) const noexcept
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
  // フィールドに置くパネルの準備
  void preparationPanel()
  {
    // 開始パネルを探す
    std::vector<int> start_panels;
    for (int i = 0; i < panels_.size(); ++i)
    {
      if (panels_[i].getAttribute() & Panel::START)
      {
        start_panels.push_back(i);
      }
    }
    assert(!start_panels.empty());

    if (start_panels.size() > 1)
    {
      // 開始パネルが何枚かある時はシャッフル
      std::shuffle(std::begin(start_panels), std::end(start_panels), engine_);
    }
    start_panel_ = start_panels[0];

    {
      // 最初に置くパネルを取り除いてからシャッフル
      auto it = std::find(std::begin(waiting_panels), std::end(waiting_panels), start_panel_);
      assert(it != std::end(waiting_panels));
      waiting_panels.erase(it);

      std::shuffle(std::begin(waiting_panels), std::end(waiting_panels), engine_);
    }

#if defined (DEBUG)
    auto force_panel = params_.getValueForKey<int>("force_panel");
    if (force_panel > 0)
    {
      // パネル枚数を強制的に変更
      waiting_panels.resize(force_panel);
    }
#endif
  }

  bool getNextPanel() noexcept
  {
    if (waiting_panels.empty()) return false;

    // 先頭から順に置けるかどうか調べる
    size_t i;
    for (i = 0; i < waiting_panels.size(); ++i)
    {
      if (canPanelPutField(panels_[waiting_panels[i]], blank_, field)) break;
    }

    if (i == waiting_panels.size())
    {
      // 全く置けない(積んだ)
      return false;
    }

    hand_panel    = waiting_panels[i];
    hand_rotation = ci::randInt(4);

    // コンテナから削除
    waiting_panels.erase(std::begin(waiting_panels) + i);

    DOUT << "Next panel: " << hand_panel << "(index: " << i << ")" << std::endl;

    return true;
  }

  // 各種情報を収集
  void fieldUpdate() noexcept
  {
    blank_ = field.searchBlank();
  }

  // スコア更新
  void updateScores() noexcept
  {
    scores_[0] = int(completed_path.size());
    scores_[1] = countTotalAttribute(completed_path, field, panels_);
    scores_[2] = int(completed_forests.size());
    scores_[3] = countTotalAttribute(completed_forests, field, panels_);
    scores_[4] = int(std::count_if(std::begin(deep_forest), std::end(deep_forest),
                                   [](int x) { return x > 0; }));
    scores_[5] = countTown(completed_path, field, panels_);
    scores_[6] = int(completed_church.size());
  }


  void calcResults() noexcept
  {
    total_score   = calcTotalScore();
    total_ranking = calcRanking(total_score);
  }

  // 最終スコア
  u_int calcTotalScore() const noexcept
  {
    // 道と森の計算用
    auto panel_rate  = Json::getVec<glm::vec2>(params_["panel_rate"]);
    auto score_rates = Json::getArray<float>(params_["score_rates"]);

    float score = 0;

    // 道の計算
    // TIPS 長い道ほど指数関数的に得点が上がる
    float path_score = 0;
    for (const auto path : completed_path)
    {
      float s = std::pow(float(path.size()), panel_rate.x) * panel_rate.y * score_rates[0];
      path_score += s;
      DOUT << path.size() << " : " << s << std::endl;
    }
    score += path_score;
    DOUT << "Path: " << path_score << std::endl;

    // 森の計算
    // TIPS 面積が大きいほど指数関数的に得点が上がる
    float forest_score = 0;
    size_t index = 0;
    for (const auto forest : completed_forests)
    {
      auto count = forest.size() + deep_forest[index] * score_rates[2];
      float s = std::pow(float(count), panel_rate.x) * panel_rate.y * score_rates[1];
      forest_score += s;
      DOUT << forest.size() << "(" << deep_forest[index] << ") : " << s << std::endl;

      ++index;
    }
    score += forest_score;
    DOUT << "Forest: " << forest_score << std::endl;

    // 深い森の数
    // float df_score = scores_[4] * score_rates[2];
    // score += df_score;
    // DOUT << "Deep forest: " << df_score << std::endl;

    // 街の数
    float town_score = scores_[5] * score_rates[3];
    score += town_score;
    DOUT << "Town forest: " << town_score << std::endl;

    // 教会
    float church_score = scores_[6] * score_rates[4];
    score += church_score;
    DOUT << "Church: " << church_score << std::endl;

    // Perfect
    if (waiting_panels.empty())
    {
      score *= params_.getValueForKey<float>("perfect_score_rate");
    }
#if defined (DEBUG)
    // テスト用にスコアを上書き
    score = Json::getValue(params_, "test_score", score);
#endif
    return score;
  }

  // ランキングを決める
  u_int calcRanking(int score) const noexcept
  {
    // NOTICE ランキングは線形に上昇する
    int rate = params_.getValueForKey<int>("ranking_rate");
    // 0は「ランク無し」
    return score / rate + 1;
  }

  // スコアを送信
  void sendScores() noexcept
  {
    double delay_time = params_.getValueForKey<double>("replay.score_delay");
    count_exec_.add(delay_time,
                    [this]() noexcept
                    {
                      Arguments args{
                        { "scores",        scores_ },
                        { "total_score",   total_score },
                        { "total_ranking", total_ranking },
                        { "total_panels",  total_panels },

                        { "completed_forest", &completed_forests },
                        { "completed_path",   &completed_path },

                        { "panel_turned_times", panel_turned_times_ },
                        { "panel_moved_times",  panel_moved_times_ },
                      };
                      event_.signal("Ranking:UpdateScores", args);
                    });
  }


  // パネルを追加してイベント送信
  void putPanel(int panel, const glm::ivec2& pos, u_int rotation) noexcept
  {
    // Panel端をここで調べる
    const auto p = panels_[panel];
    auto edge = p.getRotatedEdgeValue(rotation);
    field.addPanel(panel, pos, rotation, edge);
    //置ける場所を探す
    blank_ = field.searchBlank();

    {
      Arguments args{
        { "panel",        panel },
        { "field_pos",    pos },
        { "rotation",     rotation },
        { "total_panels", total_panels },
      };
      event_.signal("Game:PutPanel", args);
    }
  }

  // チュートリアル用のパネル順にする
  void orderForTutorial() noexcept
  {
    DOUT << "Tutorial order." << std::endl;

    // 並びが固定されているパネル順
    auto panels = Json::getArray<int>(params_["tutorial"]);

    // panelsに含まれていないパネルをコピー
    for (auto p : waiting_panels)
    {
      if (std::find(std::begin(panels), std::end(panels), p) == std::end(panels)) 
      {
        panels.push_back(p);
      }
    }

    assert(panels.size() == waiting_panels.size());
    waiting_panels = panels;
  }



  // NOTICE 変数をクラス定義の最後に書くテスト
  // FIXME 参照で持つのいくない
  const ci::JsonTree& params_;
  Event<Arguments>& event_;
  const std::vector<Panel>& panels_;

  std::mt19937 engine_;

  CountExec count_exec_;

  bool started  = false;
  bool finished = false;

  double initial_play_time_;
  double play_time_;
#if defined (DEBUG)
  bool time_count = true;
#endif

  // 配置するパネル
  std::vector<int> waiting_panels;
  // 最初に中央に配置するパネル
  int start_panel_;
  // 手持ちのパネル
  int hand_panel;
  u_int hand_rotation;

  Field field;
  
  // 列挙した置ける箇所
  std::vector<glm::ivec2> blank_;

  // 完成した森
  std::vector<std::vector<glm::ivec2>> completed_forests;
  // 深い森
  std::vector<u_int> deep_forest;
  // 完成した道
  std::vector<std::vector<glm::ivec2>> completed_path;
  // 完成した教会
  std::vector<glm::ivec2> completed_church;

  // パネルを回した回数
  u_int panel_turned_times_ = 0;
  // パネルを移動した回数
  u_int panel_moved_times_ = 0;

  // スコア
  std::vector<u_int> scores_;
  u_int total_score   = 0;
  u_int total_ranking = 0;
  u_int total_panels  = 0;

  // 最長道
  u_int max_path_ = 0;
  // 最大森
  u_int max_forest_ = 0;

#if defined (DEBUG)
public:
  // 読み込んだパス
  std::string game_path;
#endif

};

}
