#pragma once

//
// アプリ内のテキスト
// FIXME staticデータ
//

#include <string> 
#include <map>
#include <sstream>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>


namespace ngs { namespace AppText {

std::map<std::string, std::string> contents;


void init(const std::string& path)
{
  auto text = ci::loadString(Asset::load(path));

  std::vector<std::string> result;
  boost::algorithm::split(result, text, boost::is_any_of("\t\n"), boost::token_compress_on);

  for (int i = 0; i < result.size(); i += 2)
  {
    contents.insert({ result[i], result[i + 1] });
  }

  DOUT << "AppText::init" << std::endl;
}

const std::string& get(const std::string& key) 
{
  // 見つからない場合はkeyを返却
  if (!contents.count(key)) return key;
  return contents.at(key);
}

} }
