//
// アプリ内課金繋ぎ
//

#import "DCInAppPurchase.h"
#include <cinder/app/App.h>
#include "PurchaseDelegate.h"
#include "Cocoa.h"


namespace ngs {

extern std::function<void ()> purchase_completed;
extern std::function<void ()> purchase_canceled;

namespace PurchaseDelegate {

// FIXME シングルトン設計
DCInAppPurchase* inAppPurchase;

// FIXME グローバル変数禁止
std::function<void (std::string price)> price_completed;

bool checking_price = false;
bool got_price      = false;


void init(const std::function<void ()>& purchase_completed_,
          const std::function<void ()>& purchase_canceled_)
{
  inAppPurchase = [[DCInAppPurchase alloc] init];
  purchase_completed = purchase_completed_;
  purchase_canceled  = purchase_canceled_;
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

// 課金情報の問い合わせ
void price(const std::string& product_id, const std::function<void (const std::string)>& completed)
{
  // 確認中ならスルー
  if (checking_price) return;

  price_completed = completed;
  checking_price  = true;

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

    ngs::PurchaseDelegate::checking_price = false;
  }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
  NSLog(@"error");
}

@end
