#pragma once

//
// アニメーション
//

#include <boost/noncopyable.hpp>
#include <boost/any.hpp>
#include <cinder/Timeline.h>
#include "EaseFunc.hpp"


namespace ngs {

class Tween
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
    bool copy_start;

    bool enable_start;
    bool disable_end;

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

        bool loop         = Json::getValue(b, "loop", false);
        bool pingpong     = Json::getValue(b, "pingpong", false);
        float delay       = Json::getValue(b, "delay", 0.0f);
        auto ease_func    = Json::getValue(b, "ease_func", std::string("None"));
        bool copy_start   = Json::getValue(b, "copy_start", false);
        bool enable_start = Json::getValue(b, "enable_start", false);
        bool disable_end  = Json::getValue(b, "disable_end", false);

        bool has_start = false;
        boost::any start;
        boost::any end;
        if (b.hasChild("start"))
        {
          has_start = true;
          start = getValueForType(b["start"], type);
        }
        end = getValueForType(b["end"], type);

        bodies.push_back({ duration, loop, pingpong, delay, getEaseFunc(ease_func),
                           has_start, copy_start,
                           enable_start,
                           disable_end,
                           start, end });
      }

      // TODO 構造体のコピーを無くす
      components_.push_back({ param_name, type, bodies });
    }
  }

  ~Tween() = default;


  template <typename T>
  void set(const ci::TimelineRef& timeline, const T& widget) const noexcept
  {
    auto enable_func  = [widget]() noexcept
                        {
                          widget->enable();
                          DOUT << "enable: " << widget->getIdentifier() << std::endl;
                        };
    auto disable_func = [widget]() noexcept
                        {
                          widget->enable(false);
                          DOUT << "disable: " << widget->getIdentifier() << std::endl;
                        };

    for (const auto& c : components_)
    {
      const auto& param_name = c.name;
      int type = c.type;

      {
        const auto& b = c.bodies[0];
        applyTween(type, timeline, widget->getParam(param_name), b,
                   enable_func, disable_func);
        if (b.copy_start)
        {
          widget->setParam(param_name, b.start);
        }
      }

      for (u_int i = 1; i < c.bodies.size(); ++i)
      {
        const auto& b = c.bodies[i];
        appendToTween(type, timeline, widget->getParam(param_name), b,
                      enable_func, disable_func);
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

  static boost::any getValueForType(const ci::JsonTree& param, int type) noexcept
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
      return Json::getColor<float>(param);

    case Type::RECT:
      return Json::getRect<float>(param);

    default:
      return 0;
    }
  }


  template <typename T>
  static void applyTweenParams(const ci::TimelineRef& timeline,
                               const boost::any& target,
                               const Body& body,
                               const std::function<void ()>& enable_func,
                               const std::function<void ()>& disable_func) noexcept
  {
      if (body.has_start)
      {
        auto options = timeline->applyPtr(boost::any_cast<T*>(target),
                                          boost::any_cast<T>(body.start), boost::any_cast<T>(body.end),
                                          body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
        if (body.enable_start) options.startFn(enable_func);
        if (body.disable_end)  options.finishFn(disable_func);
      }
      else
      {
        auto options = timeline->applyPtr(boost::any_cast<T*>(target),
                                          boost::any_cast<T>(body.end),
                                          body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
        if (body.enable_start) options.startFn(enable_func);
        if (body.disable_end)  options.finishFn(disable_func);
      }
  }

  template <typename T>
  static void appendTweenParams(const ci::TimelineRef& timeline,
                                const boost::any& target,
                                const Body& body,
                                const std::function<void ()>& enable_func,
                                const std::function<void ()>& disable_func) noexcept
  {
      if (body.has_start)
      {
        auto options = timeline->appendToPtr(boost::any_cast<T*>(target),
                                             boost::any_cast<T>(body.start), boost::any_cast<T>(body.end),
                                             body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
        if (body.enable_start) options.startFn(enable_func);
        if (body.disable_end)  options.finishFn(disable_func);
      }
      else
      {
        auto options = timeline->appendToPtr(boost::any_cast<T*>(target),
                                             boost::any_cast<T>(body.end),
                                             body.duration, body.ease_func);
        if (body.loop)         options.loop();
        if (body.pingpong)     options.pingPong();
        if (body.delay > 0.0f) options.delay(body.delay);
        if (body.enable_start) options.startFn(enable_func);
        if (body.disable_end)  options.finishFn(disable_func);
      }
  }


  // TODO 特殊化してるだけなのをなんとかしたい
  static void applyTween(int type,
                         const ci::TimelineRef& timeline,
                         const boost::any& target,
                         const Body& body,
                         const std::function<void ()>& enable_func,
                         const std::function<void ()>& disable_func) noexcept
  {
    switch (type)
    {
    case Type::FLOAT:
      applyTweenParams<float>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::VEC2:
      applyTweenParams<glm::vec2>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::VEC3:
      applyTweenParams<glm::vec3>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::COLOR:
      applyTweenParams<ci::Color>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::RECT:
      applyTweenParams<ci::Rectf>(timeline, target, body, enable_func, disable_func);
      break;
    }
  }

  static void appendToTween(int type,
                            const ci::TimelineRef& timeline,
                            const boost::any& target,
                            const Body& body,
                            const std::function<void ()>& enable_func,
                            const std::function<void ()>& disable_func) noexcept
  {
    switch (type)
    {
    case Type::FLOAT:
      appendTweenParams<float>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::VEC2:
      appendTweenParams<glm::vec2>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::VEC3:
      appendTweenParams<glm::vec3>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::COLOR:
      appendTweenParams<ci::Color>(timeline, target, body, enable_func, disable_func);
      break;

    case Type::RECT:
      appendTweenParams<ci::Rectf>(timeline, target, body, enable_func, disable_func);
      break;
    }
  }
  
};

}
