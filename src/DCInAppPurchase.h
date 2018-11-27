//
// 課金処理
//

#import <UIKit/UIKit.h>
#import <StoreKit/StoreKit.h>


#define AI_LARGE_SIZE 50.0
#define AI_SMALL_SIZE 20.0
#define AI_BG_COLOR   [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.5f]
#define SCREEN_WIDTH  [[UIScreen mainScreen] bounds].size.width
#define SCREEN_HEIGHT [[UIScreen mainScreen] bounds].size.height


@interface DCInAppPurchase : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
  NSString*                proccessingProductId;
  id                       rootView;
  UIActivityIndicatorView* indicator;
  UIView*                  indicatorOverlay;

  BOOL isRestored;
}

typedef NS_ENUM(NSUInteger, activityIndicatorStyles)
{
  AI_GRAY        = 1,
  AI_WHITE       = 2,
  AI_WHITE_LARGE = 3
};

- (void)startPurchase:(NSString *)productId view:(id)vc;
- (void)restorePurchase:(NSString *)productId view:(id)vc;

@end
