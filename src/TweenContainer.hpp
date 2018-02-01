#pragma once

//
// Tweenのコンテナ
//

#include <boost/noncopyable.hpp>
#include "Tween.hpp"


namespace ngs {

class TweenContainer
  : private boost::noncopyable
{


public:
  struct Contents
  {
    std::string identifier;
    Tween tween;
  };

 
  TweenContainer(const ci::JsonTree& params) noexcept
  {
    for (const auto& p : params)
    {
      const auto& name = p.getKey();
      std::vector<Contents> contents;
      for (const auto& pp : p)
      {
        const auto& id = pp.getValueForKey<std::string>("identifier");

        // FIXME Tweenのコピーを無くしたい
        contents.push_back({ id, { pp["tween"] } });
      }
      // FIXME Contentsのコピーを無くしたい
      tweens_.insert({ name, contents });
    }
  }

  ~TweenContainer() = default;


  const std::vector<Contents>& at(const std::string& name) noexcept
  {
    return tweens_.at(name);
  }


private:
  std::map<std::string, std::vector<Contents>> tweens_; 


};

}
