//
// 見習い君主
//

#include "Defines.hpp"
#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include <cinder/Rand.h>
#include "Params.hpp"
#include "JsonUtil.hpp"
#include "TouchEvent.hpp"
// #include "MainPart.hpp"
#include "TestPart.hpp"
// #include "UITest.hpp"


namespace ngs {

class MyApp
  : public ci::app::App
{

public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp()
    : params(ngs::Params::load("params.json")),
      worker(params)
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
    signal_connection.disconnect();
  }
#endif


private:
  void mouseMove(ci::app::MouseEvent event) override
  {
    worker.mouseMove(event);
  }

	void mouseDown(ci::app::MouseEvent event) override
  {
    worker.mouseDown(event);
  }
  
	void mouseDrag(ci::app::MouseEvent event) override
  {
    worker.mouseDrag(event);
  }
  
	void mouseUp(ci::app::MouseEvent event) override
  {
    worker.mouseUp(event);
  }

  void mouseWheel(ci::app::MouseEvent event) override
  {
    worker.mouseWheel(event);
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


  void keyDown(ci::app::KeyEvent event) override {
    worker.keyDown(event);
  }

  void keyUp(ci::app::KeyEvent event) override {
    worker.keyUp(event);
  }
  

	void update() override {
    worker.update();
  }


  void resize() override {
    float aspect = ci::app::getWindowAspectRatio();
    worker.resize(aspect);
  }


	void draw() override {
    ci::gl::clear(ci::Color(0, 0, 0));

    auto window_size = ci::app::getWindowSize();
    worker.draw(window_size);
  }


  // 変数定義(実験的にクラス定義の最後でまとめている)
  ci::JsonTree params;

#if defined (CINDER_COCOA_TOUCH)
  ci::signals::Connection signal_connection;
#endif

  TouchEvent touch_event_;


  // MainPart worker;
  TestPart worker;

};

}


// アプリのラウンチコード
CINDER_APP(ngs::MyApp, ci::app::RendererGl,
           [](ci::app::App::Settings* settings) {
             // FIXME:ここで設定ファイルを読むなんて...
             auto params = ngs::Params::load("params.json");

             settings->setWindowSize(ngs::Json::getVec<ci::ivec2>(params["app.size"]));
             settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));

             settings->setMultiTouchEnabled();

             settings->setPowerManagementEnabled(params.getValueForKey<bool>("app.power_management"));
             settings->setHighDensityDisplayEnabled(params.getValueForKey<bool>("app.retina"));
           });
