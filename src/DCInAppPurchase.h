//
//  DCInAppPurchase.h
//
//  Created by Masaki Hirokawa on 13/09/02.
//  Copyright (c) 2013 Masaki Hirokawa. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <StoreKit/StoreKit.h>

#define AI_LARGE_SIZE       50.0
#define AI_SMALL_SIZE       20.0
#define AI_BG_COLOR         [UIColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.5f]
#define SCREEN_WIDTH        [[UIScreen mainScreen] bounds].size.width
#define SCREEN_HEIGHT       [[UIScreen mainScreen] bounds].size.height

@protocol DCInAppPurchaseDelegate;

@interface DCInAppPurchase : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver> {
    id<DCInAppPurchaseDelegate> _dc_delegate;
    id                          _delegate;
    NSString                    *proccessingProductId;
    id                          rootView;
    BOOL                        isProccessing;
    BOOL                        isRestored;
    UIActivityIndicatorView     *indicator;
    UIView                      *indicatorOverlay;
}

#pragma mark - enumerator
typedef NS_ENUM(NSUInteger, activityIndicatorStyles) {
    AI_GRAY        = 1,
    AI_WHITE       = 2,
    AI_WHITE_LARGE = 3
};

#pragma mark - property
@property (nonatomic, assign) id<DCInAppPurchaseDelegate> dc_delegate;
@property (nonatomic, assign) id delegate;

#pragma mark - public method
+ (DCInAppPurchase *)sharedManager;
- (void)startPurchase:(NSString *)productId view:(id)view;
- (void)restorePurchase:(NSString *)productId view:(id)view;

@end

#pragma mark - delegate method
@protocol DCInAppPurchaseDelegate <NSObject>
@optional
- (void)DCInAppPurchaseDidFinish;
@end
