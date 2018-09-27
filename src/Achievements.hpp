#pragma once

//
// 実績管理
// FIXME スコア管理もしている
//

#include "GameCenter.h"


namespace ngs {

class Achievements
{

public:
  Achievements(Event<Arguments>& event)
  {
    // 各種画面遷移
    holder_ += event.connect("Credits:begin",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.CREDITS");
                             });
    holder_ += event.connect("Settings:begin",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.SETTINGS");
                             });
    holder_ += event.connect("Records:begin",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.RECORDS");
                             });
    holder_ += event.connect("Ranking:begin",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.RANKING");
                             });
    holder_ += event.connect("Purchase:begin",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.PURCHASE");
                             });

    // 各種機能
    holder_ += event.connect("Share:completed",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.SHARE");
                             });

    // ゲーム内容
    holder_ += event.connect("Game:Finish",
                             [this](const Connection&, const Arguments& args)
                             {
                               // Tutorialは無視
                               auto tutorial = boost::any_cast<bool>(args.at("tutorial"));
                               if (tutorial) return;

                               GameCenter::submitAchievement("PM.CLEAR");
                             });
    holder_ += event.connect("Game:Tutorial-Finish",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.TUTORIAL");
                             });
    holder_ += event.connect("GameMain:pause",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.PAUSE");
                             });

    // スコア
    holder_ += event.connect("Result:begin",
                             [this](const Connection&, const Arguments& args)
                             {
                               // スコア送信
                               auto total_panels = boost::any_cast<u_int>(args.at("total_panels"));
                               const auto& score = boost::any_cast<const Score&>(args.at("score"));
                               GameCenter::submitScore(score.total_score, total_panels, score.total_panels);

                               auto is_tutorial = boost::any_cast<bool>(args.at("tutorial"));

                               {
                                 // ハイスコア
                                 static std::pair<const char*, double> tbl[]
                                 {
                                   { "PM.SCORE5000",    10000 },
                                   { "PM.SCORE10000",   30000 },
                                   { "PM.SCORE30000",   50000 },
                                   { "PM.SCORE50000",   70000 },
                                   { "PM.SCORE80000",  100000 },
                                   { "PM.SCORE100000", 150000 },
                                 };

                                 for (const auto& t : tbl)
                                 {
                                   auto rate = std::min((score.total_score * 100.0) / t.second, 100.0);
                                   GameCenter::submitAchievement(t.first, rate);
                                 }
                               }

                               {
                                 // 最大設置数
                                 static std::pair<const char*, double> tbl[]
                                 {
                                   { "PM.PUT30", 30.0 },
                                   { "PM.PUT40", 40.0 },
                                   { "PM.PUT50", 50.0 },
                                   { "PM.PUT60", 60.0 },
                                   { "PM.PUT70", 70.0 },
                                 };
                                 
                                 for (const auto& t : tbl)
                                 {
                                   auto rate = std::min((score.total_panels * 100.0) / t.second, 100.0);
                                   GameCenter::submitAchievement(t.first, rate);
                                 }
                               }

                               // 実績解除判定
                               if (score.perfect && !is_tutorial)
                               {
                                 // 全てのパネルを置いた
                                 GameCenter::submitAchievement("PM.PERFECT");
                               }

                               {
                                 // 最大森
                                 auto max_forest = double(boost::any_cast<u_int>(args.at("max_forest")));
                                 static std::pair<const char*, double> tbl[]
                                 {
                                   { "PM.FOREST5",   5.0 },
                                   { "PM.FOREST10", 10.0 },
                                   { "PM.FOREST15", 15.0 },
                                   { "PM.FOREST20", 20.0 },
                                 };
                                 
                                 for (const auto& t : tbl)
                                 {
                                   auto rate = std::min((max_forest * 100.0) / t.second, 100.0);
                                   GameCenter::submitAchievement(t.first, rate);
                                 }
                               }

                               {
                                 // 最大道
                                 auto max_path = double(boost::any_cast<u_int>(args.at("max_path")));
                                 static std::pair<const char*, double> tbl[]
                                 {
                                   { "PM.PATH5",   5.0 },
                                   { "PM.PATH10", 10.0 },
                                   { "PM.PATH15", 15.0 },
                                   { "PM.PATH20", 20.0 },
                                 };
                                 
                                 for (const auto& t : tbl)
                                 {
                                   auto rate = std::min((max_path * 100.0) / t.second, 100.0);
                                   GameCenter::submitAchievement(t.first, rate);
                                 }
                               }
                             });

    // 非アクティブ時にキャッシュを書き出す
    holder_ += event.connect("App:ResignActive",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::writeCachedAchievement();
                             });

#if defined (DEBUG)
    // 記録を消去
    holder_ += event.connect("Settings:Trash",
                             [this](const Connection&, const Arguments&) noexcept
                             {
                               GameCenter::resetAchievement();
                               GameCenter::writeCachedAchievement();
                             });
#endif
  }

  ~Achievements() = default;


private:
  ConnectionHolder holder_;

};

}
