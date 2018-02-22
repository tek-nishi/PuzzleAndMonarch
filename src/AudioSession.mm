//
// AudioSession設定
//

/*

setCategory一覧

NSString *const AVAudioSessionCategoryAmbient;
  別アプリでオーディオ再生中にアプリを起動しても停止しない。

NSString *const AVAudioSessionCategorySoloAmbient;
  デフォルト設定　別アプリでオーディオ再生中にアプリを起動するとオーディオが停止する。

NSString *const AVAudioSessionCategoryPlayback;
  音楽再生用のアプリで利用

NSString *const AVAudioSessionCategoryRecord;
  入力のみで録音用に利用

NSString *const AVAudioSessionCategoryPlayAndRecord;
  Voip,チャット用にマイク入力と音声出力を行う際に利用

NSString *const AVAudioSessionCategoryAudioProcessing;
  再生や録音ではなくオフラインオーディオ処理を行う際に利用。

NSString *const AVAudioSessionCategoryMultiRoute;
  USBオーディオインターフェースやHDMIなどの外部出力を接続したときに
  ヘッドホンへ別系統の音を出力できる機能です。
  
*/

#import <AVFoundation/AVFoundation.h>
#include <cinder/audio/Context.h>
#include "Defines.hpp"


namespace ngs { namespace AudioSession {

// FIXME:グローバル変数にしたくない...
static id observer = nullptr;

void begin() noexcept {
  // AVFoundationのインスタンス
  AVAudioSession* audioSession = [AVAudioSession sharedInstance];

  // カテゴリの設定
  [audioSession setCategory:AVAudioSessionCategoryAmbient error:nil];

  // AudioSession利用開始
  [audioSession setActive:YES error:nil];

  // blockを使って通知を受け取る
  observer = [[NSNotificationCenter defaultCenter] addObserverForName:AVAudioSessionRouteChangeNotification
                 object: nil
                 queue: nil
                 usingBlock: ^(NSNotification* note) {
      // ヘッドフォンの抜き差しでMONO/STEREOを切り替える
      auto ctx    = ci::audio::master();
      auto output = ctx->getOutput();
      auto device = ci::audio::Device::getDefaultOutput();

      if (device->getNumOutputChannels() != output->getNumChannels()) {
        ci::audio::Node::Format format;
        format.channelMode(ci::audio::Node::ChannelMode::MATCHES_OUTPUT);
        
        auto new_output = ctx->createOutputDeviceNode(device, format);
        ctx->setOutput(new_output);

        // 再生中のNodeを繋ぎ直す
        for (auto node : output->getInputs()) {
          if (node->isEnabled()) node >> new_output;
        }
        
        new_output->enable();
      }
    }];
}

void end() noexcept {
  AVAudioSession* audioSession = [AVAudioSession sharedInstance];
  [audioSession setActive:NO error:nil];

  if (observer) {
    [[NSNotificationCenter defaultCenter] removeObserver:observer];
    observer = nullptr;
  }
}

} }
