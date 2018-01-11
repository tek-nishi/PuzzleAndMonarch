#pragma once

//
// Json utility
// 配列から色々生成する
//

#include <sstream>
#include <cinder/Vector.h>
#include <cinder/Quaternion.h>


namespace ngs { namespace Json {

template<typename T>
std::vector<T> getArray(const ci::JsonTree& json) noexcept
{
  size_t num = json.getNumChildren();

  std::vector<T> array;
  array.reserve(num);

  for (size_t i = 0; i < num; ++i)
  {
    array.push_back(json[i].getValue<T>());
  }

  return array;
}


// FIXME:JSONからdoubleで取ってくるのが微妙
template<typename T>
T getVec(const ci::JsonTree& json) noexcept
{
  T v;
  for (size_t i = 0; i < v.length(); ++i)
  {
    v[i] = json[i].getValue<double>();
  }

  return v;
}

template<>
ci::ColorA getVec<ci::ColorA>(const ci::JsonTree& json) noexcept
{
  ci::ColorA v;
  for (size_t i = 0; i < 4; ++i)
  {
    v[i] = json[i].getValue<double>();
  }

  return v;
}

ci::Rectf getRect(const ci::JsonTree& json) noexcept
{
  return ci::Rectf(json[0].getValue<double>(), json[1].getValue<double>(),
                   json[2].getValue<double>(), json[3].getValue<double>());  
}

ci::quat getQuat(const ci::JsonTree& json)
{
  glm::vec3 v = getVec<glm::vec3>(json) * float(M_PI) / 180.0f;
  return ci::quat(v);
}


template<typename T>
ci::JsonTree createFromVec(const T& vec)
{
  ci::JsonTree json;
  for (size_t i = 0; i < vec.size(); ++i)
  {
    json.pushBack(ci::JsonTree("", vec[i]));
  }
  return json;
}

template<typename T>
ci::JsonTree createFromVec(const std::string& key, const T& vec)
{
  auto json = ci::JsonTree::makeObject(key);
  for (size_t i = 0; i < vec.size(); ++i)
  {
    json.pushBack(ci::JsonTree("", vec[i]));
  }
  return json;
}

ci::JsonTree createFromColor(const std::string& key, const ci::Color& color)
{
  auto json = ci::JsonTree::makeObject(key);
  json.pushBack(ci::JsonTree("", color.r));
  json.pushBack(ci::JsonTree("", color.g));
  json.pushBack(ci::JsonTree("", color.b));

  return json;
}

ci::JsonTree createFromColorA(const std::string& key, const ci::ColorA& color)
{
  auto json = ci::JsonTree::makeObject(key);
  json.pushBack(ci::JsonTree("", color.r));
  json.pushBack(ci::JsonTree("", color.g));
  json.pushBack(ci::JsonTree("", color.b));
  json.pushBack(ci::JsonTree("", color.a));

  return json;
}


template<typename T>
ci::ColorT<T> getColor(const ci::JsonTree& json) noexcept
{
  return ci::ColorT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>());
}

glm::vec3 getHsvColor(const ci::JsonTree& json) noexcept
{
  return glm::vec3(json[0].getValue<float>() / 360.0f, json[1].getValue<float>(), json[2].getValue<float>());
}

template<typename T>
ci::ColorAT<T> getColorA(const ci::JsonTree& json) noexcept
{
  return ci::ColorAT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>(), json[3].getValue<T>());
}

template<typename T>
ci::ColorAT<T> getColorA8(const ci::JsonTree& json) noexcept
{
  glm::vec4 value = getVec<glm::vec4>(json);
  return ci::ColorAT<T>(value / 255.0f);
}


// 初期値付き値読み込み
template<typename T>
T getValue(const ci::JsonTree& json, const std::string& name, const T& default_value) noexcept
{
  return (json.hasChild(name)) ? json[name].getValue<T>()
                               : default_value;
}

// 初期値付きベクトル読み込み
template<typename T>
T getVec(const ci::JsonTree& json, const std::string& name, const T& default_value) noexcept
{
  return (json.hasChild(name)) ? getVec<T>(json[name])
                               : default_value;
}

} }
