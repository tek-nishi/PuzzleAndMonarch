//
// text encode/decode
// 

#include "Defines.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <zlib.h>
#include <cinder/app/App.h>
#include <cinder/DataTarget.h>
#include "TextCodec.hpp"


namespace ngs { namespace TextCodec {

enum {
  OUTBUFSIZ = 1024 * 8,
};


// 圧縮
std::string encode(const std::string& input) noexcept
{
  z_stream z;
  z.zalloc = Z_NULL;
  z.zfree  = Z_NULL;
  z.opaque = Z_NULL;
  deflateInit(&z, Z_DEFAULT_COMPRESSION);

  // SOURCE:http://yak-ex.blogspot.jp/2012/12/c-advent-calendar-2012-8-c-compiler-farm.html
  z.next_in  = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.c_str()));
  z.avail_in = static_cast<unsigned int>(input.size());

  Bytef outbuf[OUTBUFSIZ];
  z.next_out  = outbuf;
  z.avail_out = OUTBUFSIZ;

  std::string output;
  while (1)
  {
    int status = deflate(&z, Z_FINISH);
    assert(status != Z_STREAM_ERROR);
    
    if ((z.avail_out == 0) || (status == Z_STREAM_END))
    {
      u_int count = OUTBUFSIZ - z.avail_out;
      output.insert(output.end(), &outbuf[0], &outbuf[count]);

      if (status == Z_STREAM_END) break;
      
      z.next_out  = outbuf;
      z.avail_out = OUTBUFSIZ;
    }
  }
  deflateEnd(&z);

  return output;
}

// 伸長
std::string decode(const std::string& input) noexcept
{
  z_stream z;
  z.zalloc = Z_NULL;
  z.zfree  = Z_NULL;
  z.opaque = Z_NULL;
  inflateInit(&z);
  
  z.next_in  = const_cast<Bytef*>(reinterpret_cast<const Bytef*>(input.c_str()));
  z.avail_in = static_cast<unsigned int>(input.size());

  Bytef outbuf[OUTBUFSIZ];
  z.next_out  = outbuf;
  z.avail_out = OUTBUFSIZ;

  std::string output;
  while (1) {
    int status = inflate(&z, Z_NO_FLUSH);
    if ((status == Z_STREAM_ERROR) || (status == Z_DATA_ERROR))
    {
      // エラーが起こった場合は空の文字列を返す
      DOUT << "decode error!!" << std::endl;
      
      inflateEnd(&z);
      return std::string();
    }

    if ((z.avail_out == 0) || (status == Z_STREAM_END))
    {
      u_int count = OUTBUFSIZ - z.avail_out;
      output.insert(output.end(), &outbuf[0], &outbuf[count]);

      if (status == Z_STREAM_END) break;

      z.next_out  = outbuf;
      z.avail_out = OUTBUFSIZ;
    }
  }
  inflateEnd(&z);

  return output;
}


// 書き出し
void write(const std::string& path, const std::string& input) noexcept
{
  auto output = encode(input);

  // Cinderにファイル書き出しが用意されていた
  auto data_ref = ci::writeFile(path);
  data_ref->getStream()->write(output);
}

// 読み込み
std::string load(const std::string& path) noexcept
{
  std::ifstream fstr(path, std::ios::binary);
  assert(fstr);

  // SOURCE: http://fa11enprince.hatenablog.com/entry/2014/04/03/233500
  std::string input((std::istreambuf_iterator<char>(fstr)),
                    std::istreambuf_iterator<char>());
    
  return decode(input);
}

} }
