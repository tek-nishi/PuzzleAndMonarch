#pragma once

//
// アプリの中核
//   ソフトリセットはこのクラスを再インスタンスすれば良い
//   画面遷移担当
//

#include <boost/noncopyable.hpp>
#include "ConnectionHolder.hpp"
#include "UIDrawer.hpp"
#include "TweenCommon.hpp"
#include "TaskContainer.hpp"
#include "MainPart.hpp"
#include "Intro.hpp"
#include "Title.hpp"
#include "Tutorial.hpp"
#include "GameMain.hpp"
#include "Result.hpp"
#include "Credits.hpp"
#include "Purchase.hpp"
#include "Settings.hpp"
#include "Records.hpp"
#include "Ranking.hpp"
#include "Archive.hpp"
#include "Sound.hpp"
#include "DebugTask.hpp"
#include "Achievements.hpp"
#include "PurchaseDelegate.h"


namespace ngs {

class Core
  : private boost::noncopyable
{

public:
  Core(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : params_(params),
      event_(event),
      achievements_(event),
      archive_("records.json", params.getValueForKey<std::string>("app.version")),
      drawer_(params["ui"]),
      tween_common_(Params::load("tw_common.json"))
  {
    // 各種イベント登録
    // Intro→Title
    holder_ += event_.connect("Intro:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                startTitle();
                              });

    // Title→GameMain
    holder_ += event_.connect("Title:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<GameMain>(params_, event_, drawer_, tween_common_);
                              });

    // Tutorial起動
    holder_ += event_.connect("Tutorial:begin",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                DOUT << "Tutorial started." << std::endl;
                                auto level = boost::any_cast<int>(args.at("level"));
                                tasks_.pushBack<Tutorial>(params_, event_, archive_, drawer_, tween_common_, level);
                              });


    holder_ += event_.connect("Game:Tutorial-Finish",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                // Title演出再び
                                title_initial_ = true;
                              });
    // Title→Credits
    holder_ += event_.connect("Credits:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<Credits>(params_, event_, drawer_, tween_common_);
                              });
    // Credits→Title
    holder_ += event_.connect("Credits:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                startTitle();
                              });
    // Title→Settings
    holder_ += event_.connect("Settings:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                Settings::Detail detail = {
                                  archive_.getRecord<bool>("bgm-enable"),
                                  archive_.getRecord<bool>("se-enable"),
                                  archive_.isSaved()
                                };

                                tasks_.pushBack<Settings>(params_, event_, drawer_, tween_common_, detail);
                              });

    // Settings→Title
    holder_ += event_.connect("Settings:Finished",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                // Settingsの変更内容を記録
                                archive_.setRecord("bgm-enable", boost::any_cast<bool>(args.at("bgm-enable")));
                                archive_.setRecord("se-enable",  boost::any_cast<bool>(args.at("se-enable")));
                                archive_.save();

                                startTitle();
                              });

    // Title→Purchase
    holder_ += event_.connect("Purchase:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<Purchase>(params_, event_, price_, drawer_, tween_common_);
                              });
    // Purchase→Title
    holder_ += event_.connect("Purchase:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                startTitle();
                              });

    // Title→Records
    holder_ += event_.connect("Records:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                Records::Detail detail = {
                                  archive_.getRecord<u_int>("play-times"),
                                  archive_.getRecord<u_int>("total-panels"),
                                  archive_.getRecord<u_int>("panel-turned-times"),
                                  archive_.getRecord<u_int>("panel-moved-times"),
                                  archive_.getRecord<u_int>("share-times"),
                                  archive_.getRecord<u_int>("startup-times"),
                                  archive_.getRecord<u_int>("abort-times"),

                                  archive_.getRecord<u_int>("max-panels"),
                                  archive_.getRecord<u_int>("max-forest"),
                                  archive_.getRecord<u_int>("max-path"),

                                  archive_.getRecord<double>("average-score"),
                                  archive_.getRecord<double>("average-put-panels"),
                                  archive_.getRecord<double>("average-moved-times"),
                                  archive_.getRecord<double>("average-turn-times"),
                                  archive_.getRecord<double>("average-put-time"),
                                };

                                tasks_.pushBack<Records>(params_, event_, drawer_, tween_common_, detail);
                              });
    // Records→Title
    holder_ += event_.connect("Records:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                startTitle();
                              });
    // Title→Ranking
    holder_ += event_.connect("Ranking:begin",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                Arguments ranking_args {
                                  { "games",      archive_.getRecordArray("games") },
                                  { "records",    archive_.existsRanking() },
                                  { "record_num", archive_.countRanking() },
                                  { "view",       true }
                                };

                                tasks_.pushBack<Ranking>(params_, event_, drawer_, tween_common_, ranking_args);
                              });
    // Ranking→Title
    holder_ += event_.connect("Ranking:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                startTitle();
                              });

    // 本編開始
    holder_ += event_.connect("Game:Start",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                              });

    // ゲーム中断
    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                startTitle();
                              });
    // GameMain→Result
    holder_ += event_.connect("Result:begin",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                tasks_.pushBack<Result>(params_, event_, drawer_, tween_common_, args);
                              });
    // Result→Title
    holder_ += event_.connect("Result:Finished",
                              [this](const Connection&, const Arguments& args) noexcept
                              {
                                bool rank_in = boost::any_cast<bool>(args.at("rank_in"));
                                if (rank_in)
                                {
                                  // TOP10に入っていたらRankingを起動
                                  Arguments ranking_args {
                                    { "games",   archive_.getRecordArray("games") },
                                    { "rank_in", rank_in },
                                    { "ranking", boost::any_cast<u_int>(args.at("ranking")) },
                                  };

                                  tasks_.pushBack<Ranking>(params_, event_, drawer_, tween_common_, ranking_args);
                                }
                                else
                                {
                                  startTitle();
                                }
                              });

    // ???
    holder_ += event.connect("Share:completed",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               archive_.addRecord("share-times", uint32_t(1));
                               archive_.save();
                             });

    // system
    holder_ += event.connect("update",
                             std::bind(&Core::update,
                                       this, std::placeholders::_1, std::placeholders::_2));

    holder_ += event.connect("App:BecomeActive",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               if (PurchaseDelegate::hasPrice()) return;

                               // ここで課金情報を取得
                               PurchaseDelegate::price("PM.PERCHASE01",
                                                       [this](const std::string price)
                                                       {
                                                         price_ = price;
                                                       });
                             });

    holder_ += event.connect("purchase-completed",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               archive_.setRecord("PM-PERCHASE01", true);
                               archive_.save();
                               DOUT << "purchase-completed"<< std::endl;
                             });

    // アプリの起動回数を更新して保存
    archive_.addRecord("startup-times", uint32_t(1));
    archive_.save();
    
    // 最初のタスクを登録
    tasks_.pushBack<Sound>(params_, event_);
    tasks_.pushBack<MainPart>(params_, event_, archive_);
    tasks_.pushBack<Intro>(params_, event_, drawer_, tween_common_);

    {
      // Sound初期設定
      auto bgm_enable = archive_.getRecord<bool>("bgm-enable");
      auto se_enable  = archive_.getRecord<bool>("se-enable");
      Arguments args{
        { "bgm-enable", bgm_enable },
        { "se-enable",  se_enable }
      };
      event_.signal("Settings:Changed", args);
    }

#if defined (DEBUG)
    // 課金ON/OFF
    holder_ += event.connect("debug-purchase",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               bool purchased = Archive::isPurchased(archive_);
                               archive_.setRecord("PM-PERCHASE01", !purchased);
                               DOUT << "debug-purchased: " << !purchased << std::endl;
                             });

#if !defined (CINDER_COCOA_TOUCH)
    tasks_.pushBack<DebugTask>(params_, event_, drawer_);
#endif

#endif
  }

  ~Core() = default;


private:
  void update(const Connection&, const Arguments& args)
  {
    auto current_time = boost::any_cast<double>(args.at("current_time"));
    auto delta_time   = boost::any_cast<double>(args.at("delta_time"));

    tasks_.update(current_time, delta_time);
  }


  void startTitle()
  {
    Title::Condition condition
    {
      title_initial_,
      archive_.isSaved(),
      archive_.existsRanking(),
      Archive::isPurchased(archive_),
      archive_.getValue("tutorial-level", 0)
    };

    tasks_.pushBack<Title>(params_, event_, drawer_, tween_common_, condition);
    // 初回起動の判定
    title_initial_ = false;
  }


  // メンバ変数を最後尾で定義する実験
  const ci::JsonTree& params_;

  Event<Arguments>& event_;
  ConnectionHolder holder_;

  TaskContainer tasks_;

  // 達成記録
  Achievements achievements_;

  // 課金の価格
  std::string price_;

  // ゲーム内記録
  Archive archive_;

  // UI
  UI::Drawer drawer_;

  TweenCommon tween_common_;

  // Title初回演出
  bool title_initial_ = true;
};

}
