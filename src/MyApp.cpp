//
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
#include "MainPart.hpp"
// #include "TestPart.hpp"
// #include "UITest.hpp"


namespace ngs {

class MyApp
  : public ci::app::App
{

  using Worker = MainPart;


public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp()
    : params(ngs::Params::load("params.json")),
      touch_event_(event_),
      worker(std::make_unique<Worker>(params, event_))
  {
    ci::Rand::randomize();

#if defined (CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    signal_connection = getSignalSupportedOrientations().connect([]() {
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
  void mouseMove(ci::app::MouseEvent event) override
  {
    worker->mouseMove(event);
  }

	void mouseDown(ci::app::MouseEvent event) override
  {
    if (event.isLeft())
    {
      touch_event_.touchBegan(event);
    }
  }
  
	void mouseDrag(ci::app::MouseEvent event) override
  {
    if (event.isLeftDown())
    {
      touch_event_.touchMoved(event);
    }
  }
  
	void mouseUp(ci::app::MouseEvent event) override
  {
    if (event.isLeft())
    {
      touch_event_.touchEnded(event);
    }
  }

  void mouseWheel(ci::app::MouseEvent event) override
  {
    worker->mouseWheel(event);
  }


  // タッチ操作ハンドリング
  void touchesBegan(ci::app::TouchEvent event) override
  {
    touch_event_.touchesBegan(event);
  }
  
  void touchesMoved(ci::app::TouchEvent event) override
  {
    touch_event_.touchesMoved(event);
  }

  void touchesEnded(ci::app::TouchEvent event) override
  {
    touch_event_.touchesEnded(event);
  }


  void keyDown(ci::app::KeyEvent event) override
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
    }
#endif

    worker->keyDown(event);
  }

  void keyUp(ci::app::KeyEvent event) override
  {
    worker->keyUp(event);
  }
  

	void update() override
  {
    worker->update();
  }


  void resize() override
  {
    worker->resize();
  }


	void draw() override {
    ci::gl::clear(ci::Color(0, 0, 0));

    auto window_size = ci::app::getWindowSize();
    worker->draw(window_size);
  }


  // 変数定義(実験的にクラス定義の最後でまとめている)
  ci::JsonTree params;

#if defined (CINDER_COCOA_TOUCH)
  ci::signals::Connection signal_connection;
#endif

  Event<Arguments> event_;

  TouchEvent touch_event_;


  // MainPart worker;
  std::unique_ptr<Worker> worker;

};

}


// アプリのラウンチコード
CINDER_APP(ngs::MyApp, ci::app::RendererGl,
           [](ci::app::App::Settings* settings) {
             // FIXME:ここで設定ファイルを読むなんて...
             auto params = ngs::Params::load("params.json");

             settings->setWindowSize(ngs::Json::getVec<ci::ivec2>(params["app.size"]));
             settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));

#if !defined (CINDER_MAC)
             settings->setMultiTouchEnabled();
#endif

             settings->setPowerManagementEnabled(params.getValueForKey<bool>("app.power_management"));
             settings->setHighDensityDisplayEnabled(params.getValueForKey<bool>("app.retina"));
           });
