#pragma once

//
// Easing関数を名前で引く
//

#include <map>
#include <cinder/Easing.h>
#include <cinder/Tween.h>


namespace ngs {

ci::EaseFn getEaseFunc(const std::string& name) noexcept
{
  const static std::map<std::string, ci::EaseFn> funcs =
  {
    { "None", ci::easeNone },

    { "InQuad",    ci::easeInQuad },
    { "OutQuad",   ci::easeOutQuad },
    { "InOutQuad", ci::easeInOutQuad },
    { "OutInQuad", ci::easeOutInQuad },
    
    { "InCubic",    ci::easeInCubic },
    { "OutCubic",   ci::easeOutCubic },
    { "InOutCubic", ci::easeInOutCubic },
    { "OutInCubic", ci::easeOutInCubic },
    
    { "InQuart",    ci::easeInQuart },
    { "OutQuart",   ci::easeOutQuart },
    { "InOutQuart", ci::easeInOutQuart },
    { "OutInQuart", ci::easeOutInQuart },
    
    { "InQuint",    ci::easeInQuint },
    { "OutQuint",   ci::easeOutQuint },
    { "InOutQuint", ci::easeInOutQuint },
    { "OutInQuint", ci::easeOutInQuint },
    
    { "InSine",    ci::easeInSine },
    { "OutSine",   ci::easeOutSine },
    { "InOutSine", ci::easeInOutSine },
    { "OutInSine", ci::easeOutInSine },
    
    { "InExpo",    ci::easeInExpo },
    { "OutExpo",   ci::easeOutExpo },
    { "InOutExpo", ci::easeInOutExpo },
    { "OutInExpo", ci::easeOutInExpo },
    
    { "InCirc",    ci::easeInCirc },
    { "OutCirc",   ci::easeOutCirc },
    { "InOutCirc", ci::easeInOutCirc },
    { "OutInCirc", ci::easeOutInCirc },

    { "InBounce",    [](const float t) { return ci::easeInBounce(t); } },
    { "OutBounce",   [](const float t) { return ci::easeOutBounce(t); } },
    { "InOutBounce", [](const float t) { return ci::easeInOutBounce(t); } },
    { "OutInBounce", [](const float t) { return ci::easeOutInBounce(t); } },

    { "InBack",    [](const float t) { return ci::easeInBack(t); } },
    { "OutBack",   [](const float t) { return ci::easeOutBack(t); } },
    { "InOutBack", [](const float t) { return ci::easeInOutBack(t); } },
    { "OutInBack", [](const float t) { return ci::easeOutInBack(t, 1.70158f); } },

    { "InElastic",    [](const float t) { return ci::easeInElastic(t, 2.0f, 1.0f); } },
    { "OutElastic",   [](const float t) { return ci::easeOutElastic(t, 2.0f, 1.0f); } },
    { "InOutElastic", [](const float t) { return ci::easeInOutElastic(t, 2.0f, 1.0f); } },
    { "OutInElastic", [](const float t) { return ci::easeOutInElastic(t, 2.0f, 1.0f); } },

    { "InAtan",    [](const float t) { return ci::easeInAtan(t); } },
    { "OutAtan",   [](const float t) { return ci::easeOutAtan(t); } },
    { "InOutAtam", [](const float t) { return ci::easeInOutAtan(t); } },
  };

  return funcs.at(name);
}

}
