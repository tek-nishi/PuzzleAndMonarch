//
// UIViewのキャプチャ(iOS専用)
// SOURCE:http://qiita.com/nasu_st/items/561d8946966015abd448
//

#include <cinder/app/Window.h>
#include <cinder/ip/Flip.h>
#include "Capture.h"


namespace ngs { namespace Capture {
  
static bool canExec(UIView* view) noexcept {
  // クラスメソッドがあるかどうかて判別している
  return [view respondsToSelector:@selector(drawViewHierarchyInRect:afterScreenUpdates:)] ? true : false;
}

bool canExec() noexcept {
  return canExec((UIView*)ci::app::getWindow()->getNative());
}


static UIImage* execute(UIView* view) noexcept {
  if ([view respondsToSelector:@selector(drawViewHierarchyInRect:afterScreenUpdates:)]) {
    // iOS7以降はOpenGLの描画も手軽にキャプチャできる
    UIGraphicsBeginImageContextWithOptions(view.bounds.size, NO, 0.0);
    
    [view drawViewHierarchyInRect:view.bounds afterScreenUpdates:NO];
    UIImage *capture = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    return capture;
  }
  else {
    // iOS6以前は...
    // キャプチャは可能だが、描画処理の最後に実行する必要がある
    // また、Cinder側でMSAAを有効にしている場合にも、
    // ひと工夫する必要がある
    return nil;

    // ci::Surface surface = ci::app::copyWindowSurface();
    // auto image = ci::cocoa::createUiImage(surface);
    // return image;
  }
}
  
UIImage* execute() noexcept {
  return execute((UIView*)ci::app::getWindow()->getNative());
}

} }
