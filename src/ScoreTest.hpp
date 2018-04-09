#pragma once

//
// スコアを確認するためのテスト
//

#if defined (DEBUG)

#include "Defines.hpp"


namespace ngs {

class ScoreTest
{

public:
  ScoreTest(Event<Arguments>& event, const std::string path)
  {
    auto full_path = getDocumentPath() / path;
    if (!ci::fs::is_regular_file(full_path))
    {
      DOUT << "No game data." << std::endl;
      return;
    }

    ci::JsonTree json;
#if defined(OBFUSCATION_GAME_RECORD)
    auto text = TextCodec::load(full_path.string());
    try
    {
      json = ci::JsonTree(text);
    }
    catch (ci::JsonTree::ExcJsonParserError& exc)
    {
      DOUT << "Game record broken." << std::endl;
      return;
    }
#else
    try
    {
      json = ci::JsonTree(ci::loadFile(full_path));
    }
    catch (ci::JsonTree::ExcJsonParserError& exc)
    {
      DOUT << "Game record broken." << std::endl;
      return;
    }
#endif

    const auto& field = json["field"];
    for (const auto& obj : field)
    {
      auto number   = obj.getValueForKey<int>("number");
      auto pos      = Json::getVec<glm::ivec2>(obj["pos"]);
      auto rotation = obj.getValueForKey<u_int>("rotation");

      Arguments args{
        { "panel", number },
        { "pos",   pos },
        { "rotation", rotation },
      };

      event.signal("Test:PutPanel", args);
    } 
  }

  ~ScoreTest() = default;



private:


};

}

#endif

