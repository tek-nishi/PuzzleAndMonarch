#pragma once

//
// アプリの中核
//   ソフトリセットはこのクラスを再インスタンスすれば良い
//

#include <boost/noncopyable.hpp>
#include "ConnectionHolder.hpp"
#include "CountExec.hpp"
#include "UIDrawer.hpp"
#include "TweenCommon.hpp"
#include "TaskContainer.hpp"
#include "MainPart.hpp"
#include "Title.hpp"
#include "GameMain.hpp"
#include "Result.hpp"
#include "Credits.hpp"
#include "Settings.hpp"


namespace ngs {

class Core
  : private boost::noncopyable
{
  void setupTask() noexcept
  {
    tasks_.clear();

    tasks_.pushBack<MainPart>(params_, event_);
    tasks_.pushBack<Title>(params_, event_, drawer_, tween_common_);
  }


public:
  Core(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : params_(params),
      event_(event),
      drawer_(params["ui"]),
      tween_common_(Params::load("tw_common.json"))
  {
    setupTask();

    // 各種イベント登録
    // Title→GameMain
    holder_ += event_.connect("Title:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<GameMain>(params_, event_, drawer_, tween_common_);
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
                                tasks_.pushBack<Title>(params_, event_, drawer_, tween_common_);
                              });
    // Title→Settings
    holder_ += event_.connect("Settings:begin",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<Settings>(params_, event_, drawer_, tween_common_);
                              });
    // Settings→Title
    holder_ += event_.connect("Settings:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<Title>(params_, event_, drawer_, tween_common_);
                              });
    // ゲーム中断
    holder_ += event_.connect("Game:Aborted",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(0.5, std::bind(&Core::setupTask, this));
                              });
    // GameMain→Result
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(2.0,
                                                [this]() {
                                                  tasks_.pushBack<Result>(params_, event_, drawer_, tween_common_);
                                                });
                              });
    // Result→Title
    holder_ += event_.connect("Result:Finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(0.5, std::bind(&Core::setupTask, this));
                              });
  }

  ~Core() = default;


  // PCで使う
  void mouseMove(const ci::app::MouseEvent& event) noexcept
  {
  }

	void mouseDown(const ci::app::MouseEvent& event) noexcept
  {
  }

	void mouseDrag(const ci::app::MouseEvent& event) noexcept
  {
  }

	void mouseUp(const ci::app::MouseEvent& event) noexcept
  {
  }

  void mouseWheel(const ci::app::MouseEvent& event) noexcept
  {
  }

  void keyDown(const ci::app::KeyEvent& event) noexcept
  {
  }

  void keyUp(const ci::app::KeyEvent& event) noexcept
  {
  }


  // 更新
  void update(const double current_time, const double delta_time) noexcept
  {
    count_exec_.update(delta_time);
    tasks_.update(current_time, delta_time);
  }

  // 描画
  void draw(const glm::ivec2& window_size) noexcept
  {
    tasks_.draw(window_size);
  }


private:
  // メンバ変数を最後尾で定義する実験
  const ci::JsonTree& params_;

  Event<Arguments>& event_;
  ConnectionHolder holder_;

  CountExec count_exec_;
  TaskContainer tasks_;

  // UI
  UI::Drawer drawer_;

  TweenCommon tween_common_;
};

}
