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
#include "MainPart.hpp"
#include "UITest.hpp"


using namespace ci;
using namespace ci::app;


namespace ngs {

class MyApp : public App {

public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp()
    : params(ngs::Params::load("params.json")),
      worker(params)
  {
    Rand::randomize();

#if defined (CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    getSignalSupportedOrientations().connect([]() {
        return ci::app::InterfaceOrientation::All;
      });
#endif
  }


private:
  void mouseMove(MouseEvent event) override {
    worker.mouseMove(event);
  }

	void mouseDown(MouseEvent event) override {
    worker.mouseDown(event);
  }
  
	void mouseDrag(MouseEvent event) override {
    worker.mouseDrag(event);
  }
  
	void mouseUp(MouseEvent event) override {
    worker.mouseUp(event);
  }

  void mouseWheel(MouseEvent event) override {
    worker.mouseWheel(event);
  }
  
  void keyDown(KeyEvent event) override {
    worker.keyDown(event);
  }

  void keyUp(KeyEvent event) override {
    worker.keyUp(event);
  }
  

	void update() override {
    worker.update();
  }


  void resize() override {
    float aspect = getWindowAspectRatio();
    worker.resize(aspect);
  }


	void draw() override {
    gl::clear(Color(0, 0, 0));

    auto window_size = getWindowSize();
    worker.draw(window_size);
  }


  // 変数定義
  JsonTree params;
  MainPart worker;
  // UITest ui_test;

};

}


// アプリのラウンチコード
CINDER_APP(ngs::MyApp, RendererGl,
           [](App::Settings* settings) {
             // FIXME:ここで設定ファイルを読むなんて...
             auto params = ngs::Params::load("params.json");

             settings->setWindowSize(ngs::Json::getVec<ci::ivec2>(params["app.size"]));
             settings->setTitle(PREPRO_TO_STR(PRODUCT_NAME));
             
             settings->setPowerManagementEnabled(true);
             settings->setHighDensityDisplayEnabled(params.getValueForKey<bool>("app.retina"));
           });
