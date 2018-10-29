//
// GameCenter操作
//

#import <GameKit/GameKit.h>
#include <sstream>
#include <iomanip>
#include <cinder/Json.h>
#include "GameCenter.h"
#include "Path.hpp"
#include "TextCodec.hpp"


namespace ngs { namespace GameCenter {

// FIXME グローバル変数を止める
static bool authenticated = false;

// FIXME グローバル変数を止める
static std::function<void()> leaderboard_finish;

void showBoard(std::function<void()> start_callback,
               std::function<void()> finish_callback)
{
  GKGameCenterViewController* gamecenter_vc = [[[GKGameCenterViewController alloc] init] autorelease];

  if (gamecenter_vc != nil)
  {
    leaderboard_finish = finish_callback;
    start_callback();
    
    auto app_vc = (UIViewController<GKGameCenterControllerDelegate>*)ci::app::getWindow()->getNativeViewController();
    gamecenter_vc.gameCenterDelegate = app_vc;
    gamecenter_vc.viewState          = GKGameCenterViewControllerStateLeaderboards;
    // [gamecenter_vc.view setNeedsDisplay];
    [app_vc presentViewController:gamecenter_vc animated:YES completion:nil];
  }  
}

// スコア送信
static void sendScore(NSArray* scores)
{
  [GKScore reportScores:scores withCompletionHandler:^(NSError* error)
      {
        if (error != nil)
        {
          NSLOG(@"Sending score error:%@", [error localizedDescription]);
        }
        else
        {
          NSLOG(@"Sending score OK!");
        }
      }];
}

static NSString* createString(const std::string& text)
{
  NSString* str = [[[NSString alloc] initWithCString:text.c_str() encoding:NSUTF8StringEncoding] autorelease];
  return str;
}

static GKScore* createScore(const std::string& id, const double value)
{
  GKScore* reporter = [[[GKScore alloc] initWithLeaderboardIdentifier:createString(id)] autorelease];
  reporter.value = value;

  return reporter;
}

// スコア送信
void submitScore(const int score, const int total_panel, const int max_panel)
{
  if (!isAuthenticated())
  {
    NSLOG(@"GameCenter::submitStageScore: GameCenter is not active.");
    return;
  }

  GKScore* score_reporter     = createScore("PM.BESTSCORES", score);
  GKScore* panels_reporter    = createScore("PM.TOTALPANEL", total_panel);
  GKScore* maxpanels_reporter = createScore("PM.MAXPANELS", max_panel);

  // GKScoreをArrayにまとめて送信
  NSArray* score_array = @[ score_reporter, panels_reporter, maxpanels_reporter ];
  sendScore(score_array);
}

static GKAchievement* getAchievementForIdentifier(const std::string& identifier)
{
  GKAchievement* achievement = [[[GKAchievement alloc] initWithIdentifier:createString(identifier)]
                                   autorelease];
  return achievement;
}


struct Achievement
{
  double rate;
  bool   submited;
};

// FIXME グローバル変数を止める
static std::map<std::string, Achievement> cached_achievements;


static void loadCachedAchievement()
{
  cached_achievements.clear();
  
  auto full_path = getDocumentPath() / "achievements.cache";
  if (!ci::fs::is_regular_file(full_path)) return;

  ci::JsonTree json;
#if defined (OBFUSCATION_ACHIEVEMENT)
  // ファイル読み込みでエラーがあった場合、空のJsonTreeを使う
  auto text_data = TextCodec::load(full_path.string());
  try
  {
    json = ci::JsonTree(text_data);
  }
  catch (ci::JsonTree::ExcJsonParserError& exc)
  {
    NSLOG(@"%s", exc.what());
  }
#else
  try
  {
    json = ci::JsonTree(ci::loadFile(full_path));
  }
  catch (ci::JsonTree::ExcJsonParserError& exc)
  {
    NSLOG(@"%s", exc.what());
  }
#endif

  std::for_each(std::begin(json), std::end(json),
                [](const auto& data)
                {
                  Achievement achievement{
                    data.getValueForKey<double>("rate"),
                    data.getValueForKey<bool>("submited"),
                    };
                  cached_achievements.insert({ data.getKey(), achievement });
                });
  
  NSLOG(@"loadCachedAchievement: %zu", json.getNumChildren());
}

void writeCachedAchievement()
{
  if (!isAuthenticated()) return;
  
  ci::JsonTree json = ci::JsonTree::makeObject();

  for (const auto& achievement : cached_achievements)
  {
    ci::JsonTree data = ci::JsonTree::makeObject(achievement.first);

    data.addChild(ci::JsonTree("rate",     achievement.second.rate))
        .addChild(ci::JsonTree("submited", achievement.second.submited));

    json.addChild(data);
  }

  if (!json.hasChildren())
  {
    NSLOG(@"NO Achievement.");
    return;
  }

  auto full_path = getDocumentPath() / "achievements.cache";
#if defined (OBFUSCATION_ACHIEVEMENT)
  TextCodec::write(full_path.string(), json.serialize());
#else
  json.write(full_path);
#endif
  
  NSLOG(@"writeCachedAchievement: %zu", json.getNumChildren());
}

static void resubmitCachedAchievement()
{
  for (auto& achievement : cached_achievements)
  {
    if (!achievement.second.submited)
    {
      submitAchievement(achievement.first, achievement.second.rate, false);
    }
  }

  NSLOG(@"resubmitCachedAchievement: done.");
}


// 達成項目の読み込み
static void loadAchievement()
{
  if (!isAuthenticated())
  {
    NSLOG(@"GameCenter::loadAchievement: GameCenter is not active.");
    return;
  }

  loadCachedAchievement();
  
  [GKAchievement loadAchievementsWithCompletionHandler:^(NSArray* achievements, NSError* error)
      {
        if (error == nil)
        {
          NSLOG(@"GameCenter::loadAchievement: done.");

          for (GKAchievement* achievement in achievements)
          {

            Achievement data{
              achievement.percentComplete,
              true
              };
            std::string id([achievement.identifier UTF8String]);
          
            cached_achievements[id] = data;
          }
          resubmitCachedAchievement();
        }
        else
        {
          NSLOG(@"loadAchievementGameCenter::Achievements: Error:%@", [error localizedDescription]);
        }
      }];
}

// 達成項目送信
void submitAchievement(const std::string& identifier, const double complete_rate, const bool banner)
{
  if (!isAuthenticated())
  {
    NSLOG(@"GameCenter::submitAchievement: GameCenter is not active.");
    return;
  }

  double submit_rate = std::min(complete_rate, 100.0);
  
  if (cached_achievements.count(identifier))
  {
    if (cached_achievements[identifier].rate >= submit_rate)
    {
      NSLOG(@"GameCenter::submitAchievement: cached.");
      return;
    }
  }

  auto& data = cached_achievements[identifier];
  data.rate     = submit_rate;
  data.submited = false;
  
  GKAchievement* achievement = getAchievementForIdentifier(identifier);
  if (achievement)
  {
    achievement.percentComplete       = submit_rate;
    achievement.showsCompletionBanner = YES;
    achievement.showsCompletionBanner = banner ? YES : NO;
    
    [GKAchievement reportAchievements:@[ achievement ] withCompletionHandler:^(NSError* error)
        {
          if (error != nil)
          {
            NSLOG(@"Error in reporting achievements:%@", [error localizedDescription]);
          }
          else
          {
            NSLOG(@"GameCenter::submitAchievement: success!!");
            // auto& data = cached_achievements[identifier];
            // data.submited = true;
          }
        }];
  }
  else
  {
    NSLOG(@"GameCenter::submitAchievement: Cant' create GKAchievement.");
  }
}


#if defined (DEBUG)
void resetAchievement()
{
  cached_achievements.clear();

  [GKAchievement resetAchievementsWithCompletionHandler:^(NSError* error)
      {
        if (error != nil)
        {
          NSLOG(@"Error in resetting achievements:%@", [error localizedDescription]);
        }
        else
        {
          NSLOG(@"deleteAchievements OK!");
        }
      }];

  // FIXME Scoreの削除機能が無い
}
#endif


// 認証
void authenticateLocalPlayer(std::function<void()> start_callback,
                             std::function<void()> finish_callback)
{
  authenticated = false;
  
  GKLocalPlayer* player = [GKLocalPlayer localPlayer];
  player.authenticateHandler = ^(UIViewController* viewController, NSError* error)
  {
    if (viewController != nil)
    {
      // 認証ダイアログ表示
      start_callback();
      UIViewController* app_vc = ci::app::getWindow()->getNativeViewController();
      [app_vc presentViewController:viewController animated:YES completion:nil];
    }
    else if ([GKLocalPlayer localPlayer].isAuthenticated)
    {
      authenticated = true;
      finish_callback();

      loadAchievement();
      
      NSLOG(@"認証成功");
    }
    else if (error != nil)
    {
      finish_callback();
      
      NSLOG(@"認証失敗");
    }
  };
}

// 認証済み？
bool isAuthenticated()
{
  return authenticated;
}

} }


// TIPS:空のクラス定義
//      実際の定義はcinderのAppCocoaTouch.mm
@interface WindowImplCocoaTouch
@end

// 既存のクラスにクラスメソッドを追加する
@interface WindowImplCocoaTouch(GameCenter)

- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController*)viewController;

@end

@implementation WindowImplCocoaTouch(GameCenter)

- (void)gameCenterViewControllerDidFinish:(GKGameCenterViewController*)viewController
{
  [viewController dismissViewControllerAnimated:YES completion:nil];
  ngs::GameCenter::leaderboard_finish();
  NSLOG(@"leaderboardViewControllerDidFinish");
}

@end
