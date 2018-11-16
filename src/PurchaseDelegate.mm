//
// アプリ内課金繋ぎ
//

#import "DCInAppPurchase.h"
#include <cinder/app/App.h>
#include "PurchaseDelegate.h"


extern std::function<void ()> purchase_completed;
extern std::function<void ()> restore_completed;

namespace ngs { namespace PurchaseDelegate {

DCInAppPurchase *inAppPurchase;

// FIXME グローバル変数禁止
std::function<void (std::string price)> price_completed;
bool got_price = false;


static NSString* createString(const std::string& text)
{
  NSString* str = [NSString stringWithUTF8String:text.c_str()];
  return str;
}


void init(const std::function<void ()>& purchase_completed_, const std::function<void ()>& restore_completed_)
{
  inAppPurchase = [[DCInAppPurchase alloc] init];
  purchase_completed = purchase_completed_;
  restore_completed  = restore_completed_;
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

bool hasPrice()
{
  return got_price;
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
    // NSLog(@"price : %@", localizedPrice);

    // NSString→std::string
    std::string p = [localizedPrice UTF8String];
    ngs::PurchaseDelegate::price_completed(p);
    ngs::PurchaseDelegate::got_price = true;
  }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
  NSLog(@"error");
}

@end
