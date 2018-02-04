﻿//
// 見習い君主
//

#include "Defines.hpp"
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>
#include "Event.hpp"
#include "Arguments.hpp"
#include "Params.hpp"
#include "JsonUtil.hpp"
#include "TouchEvent.hpp"
// #include "Core.hpp"
#include "Sandbox.hpp"
#include "Debug.hpp"


namespace ngs {

class MyApp
  : public ci::app::App,
    private boost::noncopyable
{

  // using Worker = Core;
  using Worker = Sandbox;


public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp() noexcept
  : params(ngs::Params::load("params.json")),
    touch_event_(event_),
    worker(std::make_unique<Worker>(params, event_))
  {
    ci::Rand::randomize();
    prev_time_ = getElapsedSeconds();

#if defined (DEBUG)
    debug_events_ = Debug::keyEvent(params["app.debug"]);
#endif

#if defined (CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    signal_connection = getSignalSupportedOrientations().connect([]() noexcept {
      return ci::app::InterfaceOrientation::All;
    });
#endif
  }

#if defined (CINDER_COCOA_TOUCH)
  ~MyApp()
  {
    // FIXME デストラクタ呼ばれないっぽい
    signal_connection.disconnect();
  }
#endif


private:
  void mouseMove(ci::app::MouseEvent event) noexcept override
  {
    // worker->mouseMove(event);
  }

	void mouseDown(ci::app::MouseEvent event) noexcept override
  {
    if (event.isLeft())
    {
      touch_event_.touchBegan(event);
    }
  }
  
	void mouseDrag(ci::app::MouseEvent event) noexcept override
  {
    if (event.isLeftDown())
    {
      touch_event_.touchMoved(event);
    }
  }
  
	void mouseUp(ci::app::MouseEvent event) noexcept override
  {
    if (event.isLeft())
    {
      touch_event_.touchEnded(event);
    }
  }

  void mouseWheel(ci::app::MouseEvent event) noexcept override
  {
    // worker->mouseWheel(event);
  }


  // タッチ操作ハンドリング
  void touchesBegan(ci::app::TouchEvent event) noexcept override
  {
    touch_event_.touchesBegan(event);
  }
  
  void touchesMoved(ci::app::TouchEvent event) noexcept override
  {
    touch_event_.touchesMoved(event);
  }

  void touchesEnded(ci::app::TouchEvent event) noexcept override
  {
    touch_event_.touchesEnded(event);
  }


  void keyDown(ci::app::KeyEvent event) noexcept override
  {
#if defined (DEBUG)
    auto code = event.getCode();
    switch (code)
    {
    case ci::app::KeyEvent::KEY_r:
      {
        // Soft Reset
        worker.reset();
        worker = std::make_unique<Worker>(params, event_);
      }
      break;

    case ci::app::KeyEvent::KEY_ESCAPE:
      {
        // Forced pause.
        paused_      = !paused_;
        step_update_ = false;
        DOUT << "pause: " << paused_ << std::endl;
      }
      break;

    case ci::app::KeyEvent::KEY_SPACE:
      {
        // １コマ更新
        paused_      = false;
        step_update_ = true;
      }
      break;

    default:
      {
        Debug::signalKeyEvent(event_, debug_events_, code);
      }
      break;
    }
#endif
  }

  void keyUp(ci::app::KeyEvent event) noexcept override
  {
  }


  void resize() noexcept override
  {
    event_.signal("resize", Arguments());
  }
  

	void update() noexcept override
  {
    auto current_time = getElapsedSeconds();
    auto delta_time   = current_time - prev_time_;
#if defined (DEBUG)
    if (step_update_)
    {
      delta_time = 1.0 / 60.0;
    }

    if (!paused_)
#endif
    {
      Arguments args = {
        { "current_time", current_time },
        { "delta_time",   delta_time },
      };
      event_.signal("update", args);
    }

#if defined (DEBUG)
    if (step_update_)
    {
      step_update_ = false;
      paused_      = true;
    }
#endif

    prev_time_ = current_time;
  }

	void draw() noexcept override
  {
    ci::gl::clear(ci::Color(0, 0, 0));

    Arguments args = {
      { "window_size", ci::app::getWindowSize() },
    };
    event_.signal("draw", args);
  }


  // 変数定義(実験的にクラス定義の最後でまとめている)
  ci::JsonTree params;

#if defined (CINDER_COCOA_TOUCH)
  ci::signals::Connection signal_connection;
#endif

  Event<Arguments> event_;

  TouchEvent touch_event_;

  double prev_time_;

  // MainPart worker;
  std::unique_ptr<Worker> worker;

#if defined (DEBUG)
  bool paused_      = false;
  bool step_update_ = false;

  std::map<int, std::string> debug_events_;
#endif

};


// TIPS MSVCは #ifdef 〜 #endif が使えないので関数化
void setupApp(ci::app::App::Settings* settings) noexcept
{
  // FIXME:ここで設定ファイルを読むなんて...
  auto params = ngs::Params::load("params.json");

  settings->setWindowSize(ngs::Json::getVec<ci::ivec2>(params["app.size"]));
  settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));

#if !defined (CINDER_MAC)
  settings->setMultiTouchEnabled();
#endif

#if !defined (CINDER_COCOA_TOUCH)
  if (params.getValueForKey<bool>("app.full_screen"))
  {
    settings->setFullScreen();
  }
#endif

  settings->setPowerManagementEnabled(params.getValueForKey<bool>("app.power_management"));
  settings->setHighDensityDisplayEnabled(params.getValueForKey<bool>("app.retina"));
}

}


// アプリのラウンチコード
CINDER_APP(ngs::MyApp, ci::app::RendererGl, ngs::setupApp);
