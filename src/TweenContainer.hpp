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

    Contents(std::string id, const ci::JsonTree& params) noexcept
      : identifier(std::move(id)),
        tween(params)
    {}

    ~Contents() = default;
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
        contents.emplace_back(id, pp["tween"]);
      }
      // FIXME move効いてないよね
      tweens_.emplace(name, std::move(contents));
    }
  }

  ~TweenContainer() = default;


  const std::vector<Contents>& at(const std::string& name) const
  {
    return tweens_.at(name);
  }

  std::vector<Contents>& at(const std::string& name)
  {
    return tweens_.at(name);
  }


private:
  std::map<std::string, std::vector<Contents>> tweens_; 
};

}
