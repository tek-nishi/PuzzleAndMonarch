#pragma once

//
// ゲーム内記録
//

#include <boost/noncopyable.hpp>
#include "Score.hpp"
#include "TextCodec.hpp"


namespace ngs {

class Archive
  : private boost::noncopyable
{
  void create() noexcept
  {
    auto games = ci::JsonTree::makeArray("games");
    // FIXME 初期ランクがハードコーディング
    auto g = ci::JsonTree::makeObject().addChild(ci::JsonTree("score", uint32_t(500)))
                                       .addChild(ci::JsonTree("rank",  uint32_t(0)))
    ;
    
    for (int i = 0; i < 10; ++i)
    {
      games.pushBack(g);
    }

    records_.addChild(ci::JsonTree("play-times",         uint32_t(0)))
            .addChild(ci::JsonTree("high-score",         uint32_t(0)))
            .addChild(ci::JsonTree("total-panels",       uint32_t(0)))
            .addChild(ci::JsonTree("panel-turned-times", uint32_t(0)))
            .addChild(ci::JsonTree("panel-moved-times",  uint32_t(0)))
            .addChild(ci::JsonTree("share-times",        uint32_t(0)))
            .addChild(ci::JsonTree("startup-times",      uint32_t(0)))
            .addChild(ci::JsonTree("abort-times",        uint32_t(0)))

            .addChild(ci::JsonTree("max-panels", uint32_t(0)))
            .addChild(ci::JsonTree("max-forest", uint32_t(0)))
            .addChild(ci::JsonTree("max-path",   uint32_t(0)))

            .addChild(ci::JsonTree("average-score",       0.0))
            .addChild(ci::JsonTree("average-put-panels",  0.0))
            .addChild(ci::JsonTree("average-moved-times", 0.0))
            .addChild(ci::JsonTree("average-turn-times",  0.0))
            .addChild(ci::JsonTree("average-put-time",    0.0))

            .addChild(ci::JsonTree("bgm-enable", true))
            .addChild(ci::JsonTree("se-enable",  true))

            .addChild(games)
            .addChild(ci::JsonTree("saved", false))

            .addChild(ci::JsonTree("version", version_))
    ;
  }

  void load()
  {
    if (!ci::fs::is_regular_file(full_path_))
    {
      // 記録ファイルが無い
      create();
      DOUT << "Archive:create: " << full_path_ << std::endl;
      return;
    }

#if defined(OBFUSCATION_ARCHIVE)
    auto text = TextCodec::load(full_path_.string());
    try
    {
      records_ = ci::JsonTree(text);
    }
    catch (ci::JsonTree::ExcJsonParserError&)
    {
      DOUT << "Archive broken." << std::endl;
      create();
    }
#else
    try
    {
      records_ = ci::JsonTree(ci::loadFile(full_path_));
    }
    catch (ci::JsonTree::ExcJsonParserError&)
    {
      DOUT << "Archive broken." << std::endl;
      create();
    }
#endif
    DOUT << "Archive:load: " << full_path_ << std::endl;
  }


public:
  Archive(const std::string& path, const std::string& version) noexcept
    : full_path_(getDocumentPath() / path),
      version_(version)
  {
    this->load();
  }

  ~Archive() = default;


  // Gameの記録が保存されているか？
  bool isSaved() const noexcept
  {
    return getRecord<bool>("saved");
  }

  // ランキングデータがあるか
  bool existsRanking() const
  {
    const auto& games = records_["games"];
    return std::any_of(std::begin(games), std::end(games),
                       [](const auto& it)
                       {
                         return it.hasChild("path");
                       });
  }

  // 記録されている数を調べる
  int countRanking() const
  {
    const auto& games = records_["games"];
    return (int)std::count_if(std::begin(games), std::end(games),
                              [](const auto& it)
                              {
                                return it.hasChild("path");
                              });
  }

  // プレイ結果を記録
  void recordGameResults(const Score& score, bool high_score) noexcept
  {
    // 累積記録
    addRecord("play-times",   uint32_t(1));
    addRecord("total-panels", uint32_t(score.total_panels));
    addRecord("panel-turned-times", uint32_t(score.panel_turned_times));
    addRecord("panel-moved-times",  uint32_t(score.panel_moved_times));

    if (high_score)
    {
      setRecord("high-score", uint32_t(score.total_score));
    }

    {
      // 最大設置数
      auto record = getRecord<uint32_t>("max-panels");
      setRecord("max-panels", std::max(record, score.total_panels));
    }
    {
      // 最大規模の森
      auto record = getRecord<uint32_t>("max-forest");
      auto it    = std::max_element(std::begin(score.forest), std::end(score.forest));
      auto value = uint32_t(it != std::end(score.forest) ? *it : 0);
      setRecord("max-forest", std::max(record, value));
    }
    {
      // 道最大長
      auto record = getRecord<uint32_t>("max-path");
      auto it    = std::max_element(std::begin(score.path), std::end(score.path));
      auto value = uint32_t(it != std::end(score.path) ? *it : 0);
      setRecord("max-path", std::max(record, value));
    }

    // 平均値などを計算
    auto play_times = getRecord<uint32_t>("play-times");
    {
      auto average = getRecord<double>("average-score");
      average = (average * (play_times - 1) + score.total_score) / play_times;
      setRecord("average-score", average);
    }
    {
      auto average = getRecord<double>("average-put-panels");
      average = (average * (play_times - 1) + score.total_panels) / play_times;
      setRecord("average-put-panels", average);
    }
    {
      auto average = getRecord<double>("average-moved-times");
      average = (average * (play_times - 1) + score.panel_moved_times) / play_times;
      setRecord("average-moved-times", average);
    }
    {
      auto average = getRecord<double>("average-turn-times");
      average = (average * (play_times - 1) + score.panel_turned_times) / play_times;
      setRecord("average-turn-times", average);
    }
    {
      auto average = getRecord<double>("average-put-time");

      // NOTICE １つも置けなかった場合は１つ置いた時と同じ扱い
      auto panels = std::max(score.total_panels, u_int(1));
      auto put_time = score.limit_time / double(panels);
      average = (average * (play_times - 1) + put_time) / play_times;
      setRecord("average-put-time", average);
    }

    // 記録→保存
    this->save();
  }

  
  // 記録を取得
  template <typename T>
  T getRecord(const std::string& id) const noexcept
  {
    return records_.getValueForKey<T>(id);
  }

  // 記録を変更
  template <typename T>
  void setRecord(const std::string& id, const T& value) noexcept
  {
    auto json = ci::JsonTree(id, value);
    if (records_.hasChild(id))
    {
      records_[id] = json;
    }
    else
    {
      records_.addChild(json);
      DOUT << "new record value: " << id << std::endl;
    }
  }

  // 値に加算する
  template <typename T>
  void addRecord(const std::string& id, T value) noexcept
  {
    auto v = getRecord<T>(id);
    setRecord(id, T(v + value));
  }

  void setRecordArray(const std::string&id, const ci::JsonTree& json) noexcept
  {
    records_[id] = json;
  }

  const ci::JsonTree& getRecordArray(const std::string& id) const noexcept
  {
    return records_[id];
  }

  bool hasValue(const std::string& id) const
  {
    return records_.hasChild(id);
  }

  // 初期値付き値読み取りだし
  template <typename T>
  T getValue(const std::string& id, const T& default_value) const noexcept
  {
    return (records_.hasChild(id)) ? records_.getValueForKey<T>(id)
                                   : default_value;
  }


  void save()
  {
#if defined(OBFUSCATION_ARCHIVE)
    TextCodec::write(full_path_.string(), records_.serialize());
#else
    records_.write(full_path_);
#endif
    DOUT << "Archive:write: " << full_path_ << std::endl;
  }

  // 消去
  void erase() noexcept
  {
    // 保存データの消去
    records_.clear();

    this->create();
  }


  // FIXME 特殊化処理を抽象的にするには？
  static bool isPurchased(const Archive& archive)
  {
    auto value = Json::getValue(archive.records_, "PM-PERCHASE01", false);
    DOUT << "Purchased: " << value << std::endl;

    return value;
  }

  static bool isTutorial(const Archive& archive)
  {
    auto value = Json::getValue(archive.records_, "tutorial-level", 0);
    DOUT << "Tutorial level: " << value << std::endl;
    return value >= 0;
  }


private:
  std::string version_;

  ci::fs::path full_path_;

  ci::JsonTree records_;
};

}
