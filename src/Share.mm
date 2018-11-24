//
// SNS投稿(iOS専用)
//

#include "Defines.hpp"
#include "Cocoa.h"


// 処理を付け足したいので、元クラスを継承したクラスを定義
@interface  MyUIActivityViewController: UIActivityViewController

- (void)viewWillTransitionToSize:(CGSize)size
        withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator;

@end

@implementation MyUIActivityViewController

- (void)viewWillTransitionToSize:(CGSize)size
        withTransitionCoordinator:(id<UIViewControllerTransitionCoordinator>)coordinator
{
  [super viewWillTransitionToSize:size withTransitionCoordinator:coordinator];

  // 初回は生成タイミングなのでスルーする
  if (self.beingPresented) return;

  if ([self respondsToSelector:@selector(popoverPresentationController)])
  {
    auto rect = CGRectMake(size.width / 2, size.height - 25, 1, 1);
    self.popoverPresentationController.sourceRect = rect;
  }
}

@end


namespace ngs { namespace Share {

bool canPost() noexcept
{
  bool has_class = NSClassFromString(@"UIActivityViewController") ? true : false;
  return has_class;
}

void post(const std::string& text, UIImage* image,
          std::function<void (bool completed)> complete_callback) noexcept
{
  NSString* str = createString(text);

  // FIXME:もっと賢いNSArrayの構築方法があると思う...
  NSArray* activity_items = image ? @[str, image]
                                  : @[str];
  
  MyUIActivityViewController* view_controller = [[[MyUIActivityViewController alloc] initWithActivityItems:activity_items
                                                     applicationActivities:@[]]
                                                    autorelease];

  void (^completion)(NSString* activity_type, BOOL completed, NSArray *returnedItems, NSError *activityError) = ^(NSString* activity_type, BOOL completed, NSArray *returnedItems, NSError *activityError)
  {
    DOUT << "Share::post: completion." << std::endl;
    complete_callback(activityError == nil && completed);
  };
  view_controller.completionWithItemsHandler = completion;
  
  UIViewController* app_vc = ci::app::getWindow()->getNativeViewController();

  if ([view_controller respondsToSelector:@selector(popoverPresentationController)])
  {
    // iPadではpopoverとして表示
    auto* view = app_vc.view;
    view_controller.popoverPresentationController.sourceView = view;
    auto bounds = app_vc.view.bounds;
    auto rect = CGRectMake(bounds.size.width / 2, bounds.size.height - 25, 1, 1);
    view_controller.popoverPresentationController.sourceRect = rect;
  }

  [app_vc presentViewController:view_controller animated:YES completion:nil];
}

} }
