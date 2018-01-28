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
    tasks_.update(current_time, delta_time);
  }

  // 描画
  void draw(const glm::ivec2& window_size) noexcept
  {
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
