#pragma once

//
// 実績管理
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

    // 各種機能
    holder_ += event.connect("Share:completed",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.SHARE");
                             });

    // ゲーム内容
    holder_ += event.connect("Game:Finish",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.CLEAR");
                             });
    holder_ += event.connect("Game:Tutorial-Finish",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::submitAchievement("PM.TUTORIAL");
                             });


    // 非アクティブ時にキャッシュを書き出す
    holder_ += event.connect("App:ResignActive",
                             [this](const Connection&, const Arguments&)
                             {
                               GameCenter::writeCachedAchievement();
                             });

  }

  ~Achievements() = default;


private:
  ConnectionHolder holder_;

};

}
