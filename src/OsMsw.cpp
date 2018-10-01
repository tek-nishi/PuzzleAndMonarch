//
// OSごとの差異を吸収
//

#include "Defines.hpp"
#include <cinder/app/App.h>
#include <string>
#include <map>
#include <clocale>
#include "Os.hpp"


namespace ngs { namespace Os {

std::string lang()
{
  auto l = std::string(std::setlocale(LC_ALL, ""));

  DOUT << "locale:" << l << std::endl;

  // FIXME 対応言語ごとに追加する必要がある
  std::string path{ "en.lang" };
  std::map<std::string, std::string> tbl{
    { "Japanese_Japan.932",                      "jp.lang" },
    { "Chinese (Traditional)_Hong Kong SAR.950", "zh.lang" },
  };
  if (tbl.count(l)) path = tbl.at(l);

  return path;
}

} }
