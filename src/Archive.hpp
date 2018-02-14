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
            .addChild(ci::JsonTree("total-panels", uint32_t(0)));
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
  
  void save() noexcept
  {
    records_.write(full_path_);
    DOUT << "Archive:write: " << full_path_ << std::endl;
  }


public:
  Archive(const std::string& path) noexcept
    : full_path_(getDocumentPath() / path)
  {
    load();
  }

  ~Archive() = default;


  // プレイ結果を記録
  void recordGameResults(const Score& score) noexcept
  {
    auto play_times = records_.getValueForKey<uint32_t>("play-times");
    records_["play-times"] = ci::JsonTree("play-times", uint32_t(play_times + 1));

    auto high_score = records_.getValueForKey<uint32_t>("high-score");
    if (score.total_score > high_score)
    {
      records_["high-score"] = ci::JsonTree("high-score", uint32_t(score.total_score));
    }
    
    auto total_panels = records_.getValueForKey<uint32_t>("total-panels");
    records_["total-panels"] = ci::JsonTree("total-panels", uint32_t(total_panels + score.total_panels));

    // 記録→保存
    save();
  }


  // 記録を取得
  template <typename T>
  T getRecord(const std::string& id) const noexcept
  {
    return records_.getValueForKey<T>(id);
  }


private:
  ci::fs::path full_path_;

  ci::JsonTree records_;
};

}
