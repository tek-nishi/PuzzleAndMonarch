#pragma once

//
// 共有Tween
//

#include <boost/noncopyable.hpp>
#include <map>
#include "Tween.hpp"


namespace ngs {

class TweenCommon
  : private boost::noncopyable
{
  std::map<std::string, Tween> tweens_;


public:
  TweenCommon(const ci::JsonTree& params) noexcept
  {
    for (const auto& p : params)
    {
      const auto& name = p.getKey();
      tweens_.emplace(name, p["tween"]);
    }
  }

  ~TweenCommon() = default;


  Tween& at(const std::string& name) noexcept
  {
    return tweens_.at(name);
  }

};

}
