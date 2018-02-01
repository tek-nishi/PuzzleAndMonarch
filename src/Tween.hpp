#pragma once

//
// アニメーション
//

#include <boost/noncopyable.hpp>
#include <boost/any.hpp>
#include "EaseFunc.hpp"


namespace ngs {

class Tween
  // : private boost::noncopyable
{
  // FIXME この方式だと果てがない
  enum Type {
    FLOAT,
    VEC2,
    VEC3,
    COLOR,
    RECT,
  };

  struct Body
  {
    float duration;

    bool loop;
    bool pingpong;
    float delay;

    ci::EaseFn ease_func;

    bool has_start;
    
    boost::any start;
    boost::any end;
  };

  struct Component
  {
    std::string name;
    int type;
    std::vector<Body> bodies;
  };

  std::vector<Component> components_;


public:
  Tween() = default;

  Tween(const ci::JsonTree& params) noexcept
  {
    for (int i = 0; i < params.getNumChildren(); ++i)
    {
      const auto& p = params[i];

      const auto& param_name = p.getValueForKey<std::string>("param");
      auto type = getParamType(p.getValueForKey<std::string>("type"));

      std::vector<Body> bodies;

      const auto& body = p["body"];
      for (int j = 0; j < body.getNumChildren(); ++j)
      {
        const auto& b = body[j];
        float duration = b.getValueForKey<float>("duration");

        bool loop     = Json::getValue(b, "loop", false);
        bool pingpong = Json::getValue(b, "pingpong", false);
        float delay   = Json::getValue(b, "delay", 0.0f);
        auto ease_func = Json::getValue(b, "ease_func", std::string("None"));

        bool has_start = false;
        boost::any start;
        boost::any end;
        if (b.hasChild("start"))
        {
          has_start = true;
          start = getValueForType(b["start"], type);
        }
        end = getValueForType(b["end"], type);

        bodies.push_back({ duration, loop, pingpong, delay, getEaseFunc(ease_func), has_start, start, end });
      }

      // TODO 構造体のコピーを無くす
      components_.push_back({ param_name, type, bodies });
    }
  }

  ~Tween() = default;


  template <typename T>
  void set(const ci::TimelineRef& timeline, const T& widget) const noexcept
  {
    for (const auto& c : components_)
    {
      const auto& param_name = c.name;
      int type = c.type;

      {
        const auto& b = c.bodies[0];
        applyTween(type, timeline, widget->getParam(param_name), b);
      }

      for (u_int i = 1; i < c.bodies.size(); ++i)
      {
        const auto& b = c.bodies[i];
        appendToTween(type, timeline, widget->getParam(param_name), b);
      }
    }
  }


private:
  static int getParamType(const std::string& type) noexcept
  {
    const static std::map<std::string, int> tbl = {
      { "float", Type::FLOAT },
      { "vec2",  Type::VEC2 },
      { "vec3",  Type::VEC3 },
      { "color", Type::COLOR },
      { "rect",  Type::RECT }
    };

    return tbl.at(type);
  }

  static boost::any getValueForType(const ci::JsonTree& param, const int type) noexcept
  {
    switch (type)
    {
    case Type::FLOAT:
      return param.getValue<float>();

    case Type::VEC2:
      return Json::getVec<glm::vec2>(param);

    case Type::VEC3:
      return Json::getVec<glm::vec3>(param);

    case Type::COLOR:
      return Json::getColorA<float>(param);

    case Type::RECT:
      return Json::getRect<float>(param);

    default:
      return 0;
    }
  }

  template <typename T>
  static typename ci::Tween<T>::Options applyTween(const ci::TimelineRef& timeline,
                                                   const boost::any& target,
                                                   const boost::any& start, const boost::any& end,
                                                   const float duration, const ci::EaseFn& ease_func) noexcept
  {
    return timeline->applyPtr(boost::any_cast<T*>(target), boost::any_cast<T>(start), boost::any_cast<T>(end),
                              duration, ease_func);
  }
  
  template <typename T>
  static typename ci::Tween<T>::Options applyTween(const ci::TimelineRef& timeline,
                                                   const boost::any& target,
                                                   const boost::any& end,
                                                   const float duration, const ci::EaseFn& ease_func) noexcept
  {
    return timeline->applyPtr(boost::any_cast<T*>(target), boost::any_cast<T>(end),
                              duration, ease_func);
  }
  
  template <typename T>
  static typename ci::Tween<T>::Options appendToTween(const ci::TimelineRef& timeline,
                                                      const boost::any& target,
                                                      const boost::any& start, const boost::any& end,
                                                      const float duration, const ci::EaseFn& ease_func) noexcept
  {
    return timeline->appendToPtr(boost::any_cast<T*>(target), boost::any_cast<T>(start), boost::any_cast<T>(end),
                                 duration, ease_func);
  }
  
  template <typename T>
  static typename ci::Tween<T>::Options appendToTween(const ci::TimelineRef& timeline,
                                                      const boost::any& target,
                                                      const boost::any& end,
                                                      const float duration, const ci::EaseFn& ease_func) noexcept
  {
    return timeline->appendToPtr(boost::any_cast<T*>(target), boost::any_cast<T>(end),
                                 duration, ease_func);
  }


  // TODO 特殊化してるだけなのをなんとかしたい
  static void applyTween(const int type,
                         const ci::TimelineRef& timeline,
                         const boost::any& target,
                         const Body& body) noexcept
  {
    switch (type)
    {
    case Type::FLOAT:
      if (body.has_start)
      {
        auto options = applyTween<float>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = applyTween<float>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::VEC2:
      if (body.has_start)
      {
        auto options = applyTween<glm::vec2>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = applyTween<glm::vec2>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::VEC3:
      if (body.has_start)
      {
        auto options = applyTween<glm::vec3>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = applyTween<glm::vec3>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::COLOR:
      if (body.has_start)
      {
        auto options = applyTween<ci::ColorA>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = applyTween<ci::ColorA>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::RECT:
      if (body.has_start)
      {
        auto options = applyTween<ci::Rectf>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = applyTween<ci::Rectf>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;
    }
  }

  static void appendToTween(const int type,
                            const ci::TimelineRef& timeline,
                            const boost::any& target,
                            const Body& body) noexcept
  {
    switch (type)
    {
    case Type::FLOAT:
      if (body.has_start)
      {
        auto options = appendToTween<float>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = appendToTween<float>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::VEC2:
      if (body.has_start)
      {
        auto options = appendToTween<glm::vec2>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = appendToTween<glm::vec2>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::VEC3:
      if (body.has_start)
      {
        auto options = appendToTween<glm::vec3>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = appendToTween<glm::vec3>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::COLOR:
      if (body.has_start)
      {
        auto options = appendToTween<ci::ColorA>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = appendToTween<ci::ColorA>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;

    case Type::RECT:
      if (body.has_start)
      {
        auto options = appendToTween<ci::Rectf>(timeline, target, body.start, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      else
      {
        auto options = appendToTween<ci::Rectf>(timeline, target, body.end, body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
      }
      break;
    }
  }
  
};

}
