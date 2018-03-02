#pragma once

//
// ゲーム内記録
//

#include <boost/noncopyable.hpp>
#include "Score.hpp"


namespace ngs {

class Archive
  : private boost::noncopyable
{
  void create() noexcept
  {
    records_.addChild(ci::JsonTree("play-times", uint32_t(0)))
      .addChild(ci::JsonTree("high-score", uint32_t(0)))
      .addChild(ci::JsonTree("total-panels", uint32_t(0)))
      .addChild(ci::JsonTree("bgm-enable", true))
      .addChild(ci::JsonTree("se-enable", true))
      .addChild(ci::JsonTree::makeArray("games"))
      ;
  }

  void load() noexcept
  {
    if (!ci::fs::is_regular_file(full_path_))
    {
      // 記録ファイルが無い
      create();
      DOUT << "Archive:create: " << full_path_ << std::endl;
      return;
    }

    records_ = ci::JsonTree(ci::loadFile(full_path_));
    DOUT << "Archive:load: " << full_path_ << std::endl;
  }


public:
  Archive(const std::string& path) noexcept
    : full_path_(getDocumentPath() / path)
  {
    load();
  }

  ~Archive() = default;


  // Gameの記録が保存されているか？
  bool isSaved() const noexcept
  {
    const auto& json = getRecordArray("games");
    return json.hasChildren();
  }

  // プレイ結果を記録
  void recordGameResults(const Score& score) noexcept
  {
    auto play_times = getRecord<uint32_t>("play-times");
    setRecord("play-times", uint32_t(play_times + 1));

    auto high_score = getRecord<uint32_t>("high-score");
    if (score.total_score > high_score)
    {
      setRecord("high-score", uint32_t(score.total_score));
    }
    
    auto total_panels = getRecord<uint32_t>("total-panels");
    setRecord("total-panels", uint32_t(total_panels + score.total_panels));

    // 記録→保存
    save();
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
    records_[id] = ci::JsonTree(id, value);
  }


  void setRecordArray(const std::string&id, const ci::JsonTree& json) noexcept
  {
    records_[id] = json;
  }

  const ci::JsonTree& getRecordArray(const std::string& id) const noexcept
  {
    return records_[id];
  }


  void save() noexcept
  {
    records_.write(full_path_);
    DOUT << "Archive:write: " << full_path_ << std::endl;
  }


private:
  ci::fs::path full_path_;

  ci::JsonTree records_;
};

}
