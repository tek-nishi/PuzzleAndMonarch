#include <cinder/app/App.h>
#include <cinder/app/RendererGl.h>
#include <cinder/gl/gl.h>
#include "PurchaseDelegate.h"


using namespace ci;
using namespace ci::app;

class PurchaseApp : public App
{
public:
  void setup() override;
  void update() override;
  void draw() override;

  void keyDown(KeyEvent event) override;
  void touchesBegan(TouchEvent event) override;
};


void PurchaseApp::setup()
{
  auto completed = [this]()
                   {
                     console() << "Purchase completed." << std::endl;
                   };

  ngs::PurchaseDelegate::init(completed);
  ngs::PurchaseDelegate::price("PM.PERCHASE01",
                               [this](const std::string price)
                               {
                                 console() << price << std::endl;
                               });
}

void PurchaseApp::update()
{
}

void PurchaseApp::draw()
{
  gl::clear(Color(0.5, 0.5, 0.5)); 
}


void PurchaseApp::keyDown(KeyEvent event)
{
  auto code = event.getCode();
  switch (code)
  {
  case KeyEvent::KEY_s:
    {
      console() << "Start purchase." << std::endl;
      ngs::PurchaseDelegate::start("PM.PERCHASE01");
    }
    break;

  case KeyEvent::KEY_r:
    {
      console() << "Restore purchase." << std::endl;
      ngs::PurchaseDelegate::restore("PM.PERCHASE01");
    }
    break;
    
  default:
    console() << "Key pushed." << std::endl;
    break;
  }
}

void PurchaseApp::touchesBegan(TouchEvent event)
{
#if defined(CINDER_COCOA_TOUCH)
  if (!isKeyboardVisible())
  {
    showKeyboard(KeyboardOptions().closeOnReturn(true));
  }
#endif
}


CINDER_APP(PurchaseApp, RendererGl)
