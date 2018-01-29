#pragma once

//
// アプリの中核
//   ソフトリセットはこのクラスを再インスタンスすれば良い
//

#include <boost/noncopyable.hpp>
#include "ConnectionHolder.hpp"
#include "CountExec.hpp"
#include "UIDrawer.hpp"
#include "TaskContainer.hpp"
#include "MainPart.hpp"
#include "Title.hpp"
#include "GameMain.hpp"
#include "Result.hpp"


namespace ngs {

class Core
  : private boost::noncopyable
{


public:
  Core(const ci::JsonTree& params, Event<Arguments>& event) noexcept
    : params_(params),
      event_(event),
      drawer_(params["ui"])
  {
    tasks_.pushBack<MainPart>(params, event_);
    tasks_.pushBack<Title>(params, event_, drawer_);

    // 各種イベント登録
    holder_ += event_.connect("Title:finished",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                tasks_.pushBack<GameMain>(params_, event_, drawer_);
                              });
    
    holder_ += event_.connect("abort:touch_ended",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(0.5,
                                                [this]() noexcept
                                                {
                                                  tasks_.clear();
                                
                                                  tasks_.pushBack<MainPart>(params_, event_);
                                                  tasks_.pushBack<Title>(params_, event_, drawer_);
                                                });
                              });
    
    holder_ += event_.connect("Game:Finish",
                              [this](const Connection&, const Arguments&) noexcept
                              {
                                count_exec_.add(2.0,
                                                [this]() {
                                                  tasks_.pushBack<Result>(params_, event_, drawer_);
                                                });
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
};

}
