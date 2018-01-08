#pragma once

//
// ビルドに必要な定義
//

#if defined(_MSC_VER)
// FIXME:double -> floatの暗黙の変換で出る警告
#pragma warning (disable:4244)
#pragma warning (disable:4305)
// FIXME:ポインタで示される未知のサイズ領域に対してのメモリ操作
#pragma warning (disable:4996)
// FIXME:装飾された名前が限界を超えました...という警告を抑制
#pragma warning (disable:4503)
#endif

// TIPS:console() をReleaseビルドで排除する
#ifdef DEBUG
#define DOUT ci::app::console()
#else
#define DOUT 0 && ci::app::console()
#endif

// TIPS:プリプロセッサを文字列として定義する
#define PREPRO_TO_STR(value) PREPRO_STR(value)
#define PREPRO_STR(value)    #value


#if defined(CINDER_COCOA_TOUCH)

// リリース時 NSLog 一網打尽マクロ
#ifdef DEBUG
#define NSLOG(...) NSLog(__VA_ARGS__)
#else
#define NSLOG(...) 
#endif

#endif


namespace ngs {

// 符号無し整数の別名定義
using u_char  = unsigned char;
using u_short = unsigned short;
using u_int   = unsigned int;
using u_long  = unsigned long;

}
