//
// アプリ内課金繋ぎ
//

#import "DCInAppPurchase.h"
#include <cinder/app/App.h>
#include "PurchaseDelegate.h"


extern std::function<void ()> purchase_completed;

namespace ngs { namespace PurchaseDelegate {

DCInAppPurchase *inAppPurchase;
std::function<void (std::string price)> price_completed;


static NSString* createString(const std::string& text)
{
  NSString* str = [NSString stringWithUTF8String:text.c_str()];
  return str;
}


void init(const std::function<void ()>& completed)
{
  inAppPurchase = [[DCInAppPurchase alloc] init];
  purchase_completed = completed;
}


void start(const std::string& product_id)
{
  id vc = (id)ci::app::getWindow()->getNativeViewController();
  [inAppPurchase startPurchase:createString(product_id) view:vc];
}

void restore(const std::string& product_id)
{
  id vc = (id)ci::app::getWindow()->getNativeViewController();
  [inAppPurchase restorePurchase:createString(product_id) view:vc];
}

void price(const std::string& product_id, const std::function<void (const std::string)>& completed)
{
  price_completed = completed;

  id vc = (id<SKProductsRequestDelegate>)ci::app::getWindow()->getNativeViewController();
  SKProductsRequest* request = [[[SKProductsRequest alloc] initWithProductIdentifiers: [NSSet setWithObject: createString(product_id)]] autorelease];
  request.delegate = vc;
  [request start];
}

} }


// TIPS:空のクラス定義
//      実際の定義はcinderのAppCocoaTouch.mm
@interface WindowImplCocoaTouch
@end

// 既存のクラスにクラスメソッドを追加する
@interface WindowImplCocoaTouch(Purchase)

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;
- (void)request:(SKRequest *)request didFailWithError:(NSError *)error;

@end

@implementation WindowImplCocoaTouch(Purchase)

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
  for (SKProduct* product in response.products)
  {
    // ローカライズされたプロダクト情報
    // NSString* displayTitle = product.localizedTitle;
    // NSString* displayDesc  = product.localizedDescription;

    NSNumberFormatter* numberFormatter = [[[NSNumberFormatter alloc] init] autorelease];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
    [numberFormatter setLocale:product.priceLocale];

    NSString* localizedPrice = [numberFormatter stringFromNumber:product.price];
    NSLog(@"price : %@", localizedPrice);

    // NSString→std::string
    std::string p = [localizedPrice UTF8String];
    ngs::PurchaseDelegate::price_completed(p);
  }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
  NSLog(@"error");
}

@end
