//
// SNS投稿(iOS専用)
//

#import "Defines.hpp"


namespace ngs { namespace Share {

bool canPost() noexcept {
  bool has_class = NSClassFromString(@"UIActivityViewController") ? true : false;
  return has_class;
}


static NSString* createString(const std::string& text) {
  return [[[NSString alloc] initWithCString:text.c_str() encoding:NSUTF8StringEncoding] autorelease];
}

  
void post(const std::string& text, UIImage* image,
          std::function<void()> complete_callback) noexcept {

  NSString* str = createString(text);

  // FIXME:もっと賢いNSArrayの構築方法があると思う...
  NSArray* activity_items = image ? @[str, image]
                                  : @[str];
  
  UIActivityViewController* view_controller = [[[UIActivityViewController alloc] initWithActivityItems:activity_items
                                                                                 applicationActivities:@[]]
                                                  autorelease];

  void (^completion)(NSString* activity_type, BOOL completed, NSArray *returnedItems, NSError *activityError) = ^(NSString* activity_type, BOOL completed, NSArray *returnedItems, NSError *activityError) {
    DOUT << "Share::completion." << std::endl;
    complete_callback();
  };
  view_controller.completionWithItemsHandler = completion;
  
  UIViewController* app_vc = ci::app::getWindow()->getNativeViewController();

  if ([view_controller respondsToSelector:@selector(popoverPresentationController)]) {
    // iPadではpopoverとして表示
    view_controller.popoverPresentationController.sourceView = app_vc.view;
  }
  
  [app_vc presentViewController:view_controller animated:YES completion:nil];
}

} }
