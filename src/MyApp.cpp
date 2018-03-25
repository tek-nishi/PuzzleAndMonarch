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
#include "Core.hpp"
// #include "Sandbox.hpp"
#include "Debug.hpp"


namespace ngs {

class MyApp
  : public ci::app::App,
    private boost::noncopyable
{
  using Worker = Core;
  // using Worker = Sandbox;


public:
  // TIPS cinder 0.9.1はコンストラクタが使える
  MyApp() noexcept
  : params_(Params::load("params.json")),
    touch_event_(event_),
    worker_(std::make_unique<Worker>(params_, event_))
  {
    DOUT << "Window size: " << getWindowSize() << std::endl;
    DOUT << "Resolution:  " << ci::app::toPixels(getWindowSize()) << std::endl;

    ci::Rand::randomize();

#if defined (DEBUG)
    debug_events_ = Debug::keyEvent(params_["app.debug"]);
#endif

    if (!isFrameRateEnabled())
    {
      ci::gl::enableVerticalSync();
      DOUT << "enableVerticalSync." << std::endl;
    }

#if defined (CINDER_COCOA_TOUCH)
    // 縦横画面両対応
    getSignalSupportedOrientations().connect([]() noexcept
                                             {
                                               return ci::app::InterfaceOrientation::All;
                                             });

    getSignalWillRotate().connect([this]() noexcept
                                  {
                                    if (pending_draw_)
                                    {
                                      // １回だけ更新
                                      pending_draw_      = false;
                                      pending_draw_next_ = true;
                                    }
                                    DOUT << "SignalWillRotate" << std::endl;
                                  });

    // getSignalWillEnterForeground().connect([this]() noexcept
    //                                        {
    //                                          DOUT << "WillEnterForeground" << std::endl;
    //                                        });

#endif
    
    // アクティブになった時にタッチ情報を初期化
    getSignalDidBecomeActive().connect([this]() noexcept
                                       {
                                         // 時間経過を再初期化
                                         prev_time_ = getElapsedSeconds();
                                         event_.signal("App:BecomeActive", Arguments());
                                         DOUT << "SignalDidBecomeActive" << std::endl;
                                       });

    // 非アクティブ時
    getSignalWillResignActive().connect([this]() noexcept
                                        {
                                          event_.signal("App:ResignActive", Arguments());
                                          DOUT << "SignalWillResignActive" << std::endl;
                                        });

    event_.connect("App:pending-update",
                   [this](const Connection&, const Arguments&) noexcept
                   {
                     pending_update_ = true;
                     // pending_draw_   = true;
                     // TIPS 画面を暗くしてから描画を止めるため、予約だけしておく
                     pending_draw_next_ = true;
                   });

    event_.connect("App:resume-update",
                   [this](const Connection&, const Arguments&) noexcept
                   {
                     prev_time_ = getElapsedSeconds();
                     pending_update_    = false;
                     pending_draw_      = false;
                     pending_draw_next_ = false;
                   });

    prev_time_ = getElapsedSeconds();
  }

  ~MyApp() = default;


private:
	void mouseDown(ci::app::MouseEvent event) noexcept override
  {
    if (event.isLeft())
    {
      if (event.isAltDown())
      {
        // 擬似マルチタッチ操作
        touch_event_.multiTouchBegan(event);
      }
      else
      {
        touch_event_.touchBegan(event);
      }
    }
  }
  
	void mouseDrag(ci::app::MouseEvent event) noexcept override
  {
    if (event.isLeftDown())
    {
      if (event.isAltDown())
      {
        // 擬似マルチタッチ操作
        touch_event_.multiTouchMoved(event);
      }
      else
      {
        touch_event_.touchMoved(event);
      }
    }
  }
  
	void mouseUp(ci::app::MouseEvent event) noexcept override
  {
    if (event.isLeft())
    {
      touch_event_.touchEnded(event);
    }
  }

  // void mouseWheel(ci::app::MouseEvent event) noexcept override
  // {
  // }


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


#if defined (DEBUG)
  void keyDown(ci::app::KeyEvent event) noexcept override
  {
    auto code = event.getCode();
    switch (code)
    {
    case ci::app::KeyEvent::KEY_r:
      {
        // Soft Reset
        params_ = Params::load("params.json");
        worker_.reset();
        worker_ = std::make_unique<Worker>(params_, event_);
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
  }

  void keyUp(ci::app::KeyEvent event) noexcept override
  {
  }
#endif


  void resize() noexcept override
  {
    event_.signal("resize", Arguments());
  }
  

	void update() noexcept override
  {
    if (pending_update_) return;

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
    if (pending_draw_) return;

    ci::gl::clear(ci::Color::black());

    Arguments args = {
      { "window_size", ci::app::getWindowSize() },
    };
    event_.signal("draw", args);

    pending_draw_ = pending_draw_next_;
  }


  // 変数定義(実験的にクラス定義の最後でまとめている)
  ci::JsonTree params_;
  Event<Arguments> event_;
  TouchEvent touch_event_;

  double prev_time_;

  std::unique_ptr<Worker> worker_;

#if defined (DEBUG)
  bool paused_      = false;
  bool step_update_ = false;

  std::map<int, std::string> debug_events_;
#endif

  // Share機能とかで内部更新を保留にする
  bool pending_update_    = false;
  bool pending_draw_      = false;
  bool pending_draw_next_ = false;
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

#if defined (CINDER_COCOA_TOUCH)
  // Night shiftなどで処理速度が落ちるので
  // FrameRateは固定
  settings->disableFrameRate();
#else
  // PC版は設定で変更可能
  float frame_rate = params.getValueForKey<float>("app.frame_rate");
  (frame_rate > 0.0f) ? settings->setFrameRate(frame_rate)
                      : settings->disableFrameRate();
#endif
}

}


// アプリのラウンチコード
CINDER_APP(ngs::MyApp, ci::app::RendererGl, ngs::setupApp);
