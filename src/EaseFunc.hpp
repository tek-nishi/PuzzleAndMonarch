#pragma once

//
// Easing関数を名前で引く
//

#include <map>
#include <cinder/Easing.h>
#include <cinder/Tween.h>


namespace ngs {

ci::EaseFn getEaseFunc(const std::string& name) noexcept;


#if defined(NGS_EASEFUNC_IMPLEMENTATION)

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

    { "InBounce",    [](float t) noexcept { return ci::easeInBounce(t); } },
    { "OutBounce",   [](float t) noexcept { return ci::easeOutBounce(t); } },
    { "InOutBounce", [](float t) noexcept { return ci::easeInOutBounce(t); } },
    { "OutInBounce", [](float t) noexcept { return ci::easeOutInBounce(t); } },

    { "InBack",    [](float t) noexcept { return ci::easeInBack(t); } },
    { "OutBack",   [](float t) noexcept { return ci::easeOutBack(t); } },
    { "InOutBack", [](float t) noexcept { return ci::easeInOutBack(t); } },
    { "OutInBack", [](float t) noexcept { return ci::easeOutInBack(t, 1.70158f); } },

    { "InElastic",    [](float t) noexcept { return ci::easeInElastic(t, 2.0f, 1.0f); } },
    { "OutElastic",   [](float t) noexcept { return ci::easeOutElastic(t, 2.0f, 1.0f); } },
    { "InOutElastic", [](float t) noexcept { return ci::easeInOutElastic(t, 2.0f, 1.0f); } },
    { "OutInElastic", [](float t) noexcept { return ci::easeOutInElastic(t, 2.0f, 1.0f); } },

    { "InAtan",    [](float t) noexcept { return ci::easeInAtan(t); } },
    { "OutAtan",   [](float t) noexcept { return ci::easeOutAtan(t); } },
    { "InOutAtan", [](float t) noexcept { return ci::easeInOutAtan(t); } },
  };

  return funcs.at(name);
}

#endif

}
