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
#include "UITest.hpp"


using namespace ci;
using namespace ci::app;


namespace ngs {

class MyApp : public App {

public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp()
    : params(ngs::Params::load("params.json")),
      ui_test(params)
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
  }

	void mouseDown(MouseEvent event) override {
  }
  
	void mouseDrag(MouseEvent event) override {
  }
  
	void mouseUp(MouseEvent event) override {
  }

  void mouseWheel(MouseEvent event) override {
  }
  
  void keyDown(KeyEvent event) override {
  }

  void keyUp(KeyEvent event) override {
  }
  

	void update() override {
  }


  void resize() override {
    float aspect = getWindowAspectRatio();
    ui_test.resize(aspect);
  }


	void draw() override {
    gl::clear(Color(0, 0, 0));

    auto window_size = getWindowSize();
    ui_test.draw(window_size);
  }


  // 変数定義
  JsonTree params;
  UITest ui_test;

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
             settings->setHighDensityDisplayEnabled(false);
           });
