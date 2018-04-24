#pragma once

//
// Json utility
// 配列から色々生成する
//

#include <sstream>
#include <cinder/Vector.h>
#include <cinder/Quaternion.h>


namespace ngs { namespace Json {

template <typename T>
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
template <typename T>
T getVec(const ci::JsonTree& json) noexcept
{
  T v;
  for (int i = 0; i < v.length(); ++i)
  {
    v[i] = json[i].getValue<double>();
  }

  return v;
}

template <>
ci::ColorA getVec<ci::ColorA>(const ci::JsonTree& json) noexcept
{
  ci::ColorA v;
  for (u_int i = 0; i < 4; ++i)
  {
    v[i] = json[i].getValue<double>();
  }

  return v;
}

ci::quat getQuat(const ci::JsonTree& json) noexcept
{
  glm::vec3 v = getVec<glm::vec3>(json) * float(M_PI) / 180.0f;
  return ci::quat(v);
}

template <typename T>
ci::JsonTree createArray(const T& array) noexcept
{
  ci::JsonTree json;
  for (size_t i = 0; i < array.size(); ++i)
  {
    json.pushBack(ci::JsonTree("", array[i]));
  }
  return json;
}

template <typename T>
ci::JsonTree createArray(const std::string& key, const T& array) noexcept
{
  ci::JsonTree json = ci::JsonTree::makeObject(key);
  for (size_t i = 0; i < array.size(); ++i)
  {
    json.pushBack(ci::JsonTree("", array[i]));
  }
  return json;
}

template <typename T>
ci::JsonTree createFromVec(const T& vec) noexcept
{
  ci::JsonTree json;
  for (int i = 0; i < vec.length(); ++i)
  {
    json.pushBack(ci::JsonTree("", vec[i]));
  }
  return json;
}

template <typename T>
ci::JsonTree createFromVec(const std::string& key, const T& vec) noexcept
{
  auto json = ci::JsonTree::makeObject(key);
  for (int i = 0; i < vec.length(); ++i)
  {
    json.pushBack(ci::JsonTree("", vec[i]));
  }
  return json;
}

ci::JsonTree createFromColor(const std::string& key, const ci::Color& color) noexcept
{
  auto json = ci::JsonTree::makeObject(key);
  json.pushBack(ci::JsonTree("", color.r));
  json.pushBack(ci::JsonTree("", color.g));
  json.pushBack(ci::JsonTree("", color.b));

  return json;
}

ci::JsonTree createFromColorA(const std::string& key, const ci::ColorA& color) noexcept
{
  auto json = ci::JsonTree::makeObject(key);
  json.pushBack(ci::JsonTree("", color.r));
  json.pushBack(ci::JsonTree("", color.g));
  json.pushBack(ci::JsonTree("", color.b));
  json.pushBack(ci::JsonTree("", color.a));

  return json;
}


template <typename T>
ci::ColorT<T> getColor(const ci::JsonTree& json) noexcept
{
  return ci::ColorT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>());
}

glm::vec3 getHsvColor(const ci::JsonTree& json) noexcept
{
  return glm::vec3(json[0].getValue<float>() / 360.0f, json[1].getValue<float>(), json[2].getValue<float>());
}

template <typename T>
ci::ColorAT<T> getColorA(const ci::JsonTree& json) noexcept
{
  return ci::ColorAT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>(), json[3].getValue<T>());
}

template <typename T>
ci::ColorAT<T> getColorA8(const ci::JsonTree& json) noexcept
{
  glm::vec4 value = getVec<glm::vec4>(json);
  return ci::ColorAT<T>(value / 255.0f);
}

template <typename T>
ci::RectT<T> getRect(const ci::JsonTree& json) noexcept
{
  return ci::RectT<T>(json[0].getValue<T>(), json[1].getValue<T>(), json[2].getValue<T>(), json[3].getValue<T>());
}


// 初期値付き値読み込み
template <typename T>
T getValue(const ci::JsonTree& json, const std::string& name, const T& default_value) noexcept
{
  return (json.hasChild(name)) ? json[name].getValue<T>()
                               : default_value;
}

// 初期値付きベクトル読み込み
template <typename T>
T getVec(const ci::JsonTree& json, const std::string& name, const T& default_value) noexcept
{
  return (json.hasChild(name)) ? getVec<T>(json[name])
                               : default_value;
}

// コンテナ→JsonTree
template <typename T>
ci::JsonTree createVecArray(const std::string& key, const T& array) noexcept
{
  ci::JsonTree json = ci::JsonTree::makeObject(key);

  for (const auto& obj : array)
  {
    json.pushBack(createFromVec(obj));
  }

  return json;
}

template <typename T>
ci::JsonTree createVecVecArray(const std::string& key, const T& array) noexcept
{
  ci::JsonTree json = ci::JsonTree::makeObject(key);

  for (const auto& aa : array)
  {
    ci::JsonTree j;
    for (const auto& obj : aa)
    {
      j.pushBack(createFromVec(obj));
    }
    json.pushBack(j);
  }

  return json;
}

template <typename T>
std::vector<T> getVecArray(const ci::JsonTree& json) noexcept
{
  std::vector<T> array;

  for (const auto& obj : json)
  {
    array.push_back(getVec<T>(obj));
  }

  return array;
}

template <typename T>
std::vector<std::vector<T>> getVecVecArray(const ci::JsonTree& json) noexcept
{
  std::vector<std::vector<T>> array;

  for (const auto& aa : json)
  {
    std::vector<T> ar;
    for (const auto& obj : aa)
    {
      ar.push_back(getVec<T>(obj));
    }
    array.push_back(ar);
  }

  return array;
}

// TIPS std::sortが使えないので自前で用意
void sort(ci::JsonTree& json, std::function<bool (const ci::JsonTree& a, const ci::JsonTree& b)> comp) noexcept
{
  if (!json.hasChildren()) return;

  size_t num = json.getNumChildren();
  for (size_t i = 0; i < (num - 1); ++i)
  {
    for (size_t j = (i + 1); j < num; ++j)
    {
      if (comp(json[i], json[j]))
      {
        std::swap(json[i], json[j]);
      }
    }
  }
}

} }
