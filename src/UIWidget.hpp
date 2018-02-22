#pragma once

//
// UI Widget
//

#include "UIWidgetBase.hpp"


namespace ngs { namespace UI {

// TIPS 自分自身を引数に取る関数があるので先行宣言が必要
class Widget;
using WidgetPtr = std::shared_ptr<Widget>;


class Widget
  : private boost::noncopyable
{

  
public:
  Widget(const ci::Rectf& rect) noexcept
    : rect_(rect)
  {
#if defined (DEBUG)
    // 枠線のために適当に色を決める
    disp_color_ = ci::hsvToRgb(glm::vec3(ci::randFloat(), 1.0f, 1.0f));
#endif
  }

  ~Widget() = default;


  const std::string& getIdentifier() const noexcept
  {
    return identifier_;
  }

  bool hasIdentifier() const noexcept
  {
    return !identifier_.empty();
  }

  const std::string& getEvent() const noexcept
  {
    return event_;
  }

  bool hasEvent() const noexcept
  {
    return has_event_;
  }
  
  const std::string& getSe() const noexcept
  {
    return se_;
  }

  bool hasSe() const noexcept
  {
    return has_se_;
  }


  bool hasChild() const noexcept
  {
    return !children_.empty();
  }

  void addChildren(const WidgetPtr& widget) noexcept
  {
    children_.push_back(widget);
  }

  const std::vector<WidgetPtr>& getChildren() const noexcept
  {
    return children_;
  }

  void setWidgetBase(std::unique_ptr<UI::WidgetBase>&& base) noexcept
  {
    widget_base_ = std::move(base);
  }

  // FIXME 直前の描画結果から判定している
  bool contains(const glm::vec2& point) const noexcept
  {
    return parent_enable_
           && enable_
           && disp_rect_.contains(point);
  }

  // 画面に表示するかどうかの制御
  //   全ての子供にも影響
  void enable(bool enable = true) noexcept
  {
    enable_ = enable;
    // 再帰的に設定

    if (children_.empty()) return;

    for (const auto& child : children_)
    {
      child->parentEnable(enable);
    }
  }

  bool isEnable() const noexcept
  {
    return enable_;
  }


  // パラメーター設定
  void setParam(const std::string& name, const boost::any& value) noexcept
  {
    // UI::Widget内
    std::map<std::string, std::function<void (const boost::any& v)>> tbl = {
      { "rect", [this](const boost::any& v) noexcept
                {
                  rect_ = boost::any_cast<const ci::Rectf&>(v);
                }
      },
      { "pivot", [this](const boost::any& v) noexcept
                 {
                   pivot_ = boost::any_cast<const glm::vec2&>(v);
                 }
      },
      { "anchor_min", [this](const boost::any& v) noexcept
                      {
                        anchor_min_ = boost::any_cast<const glm::vec2&>(v);
                      }
      },
      { "anchor_max", [this](const boost::any& v) noexcept
                      {
                        anchor_max_ = boost::any_cast<const glm::vec2&>(v);
                      }
      },
      { "offset", [this](const boost::any& v) noexcept
                  {
                    offset_ = boost::any_cast<const glm::vec2&>(v);
                  }
      },
      { "scale", [this](const boost::any& v) noexcept
                 {
                   scale_ = boost::any_cast<const glm::vec2&>(v);
                 }
      },
      { "alpha", [this](const boost::any& v) noexcept
                 {
                   alpha_ = boost::any_cast<float>(v);
                 }
      }
    };

    if (tbl.count(name))
    {
      // TIPS コンテナを関数ポインタとして利用
      tbl.at(name)(value);
    }
    else
    {
      //
      widget_base_->setParam(name, value);
    }
  }

  // パラメータ取得(Pointerで返却する)
  boost::any getParam(const std::string& name) noexcept
  {
    // UI::Widget内
    std::map<std::string, std::function<boost::any ()>> tbl = {
      { "rect", [this]() noexcept
                {
                  return &rect_;
                }
      },
      { "pivot", [this]() noexcept
                 {
                   return &pivot_;
                 }
      },
      { "anchor_min", [this]() noexcept
                      {
                        return &anchor_min_;
                      }
      },
      { "anchor_max", [this]() noexcept
                      {
                        return &anchor_max_;
                      }
      },
      { "offset", [this]() noexcept
                  {
                    return &offset_;
                  }
      },
      { "scale", [this]() noexcept
                 {
                   return &scale_;
                 }
      },
      { "alpha", [this]() noexcept
                 {
                   return &alpha_;
                 }
      }
    };

    if (tbl.count(name))
    {
      // TIPS コンテナを関数ポインタとして利用
      return tbl.at(name)();
    }
    else
    {
      return widget_base_->getParam(name);
    }
  }


  void draw(const ci::Rectf& parent_rect, const glm::vec2& parent_scale,
            UI::Drawer& drawer, float parent_alpha) noexcept
  {
    if (!enable_) return;

    auto scale = parent_scale * scale_;
    auto alpha = parent_alpha * alpha_;
    disp_rect_ = calcRect(parent_rect, scale);
    widget_base_->draw(disp_rect_, drawer, alpha);

    for (const auto& child : children_)
    {
      child->draw(disp_rect_, scale, drawer, alpha);
    }
  }

#if defined (DEBUG)
  // デバッグ用にRectを描画
  void debugDraw() const noexcept
  {
    if (!enable_) return;

    ci::gl::color(disp_color_);
    ci::gl::drawStrokedRect(disp_rect_);

    for (const auto& child : children_)
    {
      child->debugDraw();
    }
  }
#endif


  // パラメーターから生成
  static WidgetPtr createFromParams(const ci::JsonTree& params) noexcept
  {
    auto rect = Json::getRect(params["rect"]);
    auto widget = std::make_shared<UI::Widget>(rect);

    if (params.hasChild("identifier"))
    {
      widget->identifier_ = params.getValueForKey<std::string>("identifier");
    }
    if (params.hasChild("enable"))
    {
      widget->enable_ = params.getValueForKey<bool>("enable");
    }
    if (params.hasChild("alpha"))
    {
      widget->alpha_ = params.getValueForKey<float>("alpha");
    }
    if (params.hasChild("anchor"))
    {
      widget->anchor_min_ = Json::getVec<glm::vec2>(params["anchor"][0]);
      widget->anchor_max_ = Json::getVec<glm::vec2>(params["anchor"][1]);
    }
    if (params.hasChild("scale"))
    {
      widget->scale_ = Json::getVec<glm::vec2>(params["scale"]);
    }
    if (params.hasChild("pivot"))
    {
      widget->pivot_ = Json::getVec<glm::vec2>(params["pivot"]);
    }
    if (params.hasChild("event"))
    {
      widget->event_     = params.getValueForKey<std::string>("event");
      widget->has_event_ = true;
    }
    if (params.hasChild("se"))
    {
      widget->se_     = params.getValueForKey<std::string>("se");
      widget->has_se_ = true;
    }

    return widget;
  }


  // 非表示なWidgetの検査
  static void checkInactiveWidget(const WidgetPtr& widget) noexcept
  {
    if (!widget->isEnable())
    {
      widget->enable(false);
    }

    if (widget->children_.empty()) return;

    for (const auto& child : widget->children_)
    {
      checkInactiveWidget(child);
    }
  }


private:
  // 親の情報から自分の位置、サイズを計算
  ci::Rectf calcRect(const ci::Rectf& parent_rect, const glm::vec2& scale) const noexcept
  {
    auto rect = rect_.getOffset(offset_);

    glm::vec2 parent_size = parent_rect.getSize();

    // 親のサイズとアンカーから左下・右上の座標を計算
    glm::vec2 anchor_min = parent_size * anchor_min_;
    glm::vec2 anchor_max = parent_size * anchor_max_;

    // 相対座標(スケーリング抜き)
    glm::vec2 pos  = rect.getUpperLeft()  + anchor_min;
    glm::vec2 size = rect.getLowerRight() + anchor_max - pos;

    // pivotを考慮したスケーリング
    glm::vec2 d = size * pivot_;
    pos -= d * scale - d;
    size *= scale;

    glm::vec2 parent_pos = parent_rect.getUpperLeft();
    return ci::Rectf(pos + parent_pos, pos + size + parent_pos);
  }
  
  void parentEnable(bool enable) noexcept
  {
    parent_enable_ = enable;
  }


  // メンバ変数
  std::string identifier_;
  bool enable_ = true;
  // 親も有効
  bool parent_enable_ = true;

  ci::Rectf rect_;
  glm::vec2 offset_ = { 0.0f, 0.0f };

  // スケーリングの中心(normalized)
  glm::vec2 pivot_ = { 0.5f, 0.5f };

  // 親のサイズの影響力(normalized)
  glm::vec2 anchor_min_ = { 0.5f, 0.5f };
  glm::vec2 anchor_max_ = { 0.5f, 0.5f };

  glm::vec2 scale_ = { 1.0f, 1.0f };

  bool has_event_ = false;
  std::string event_;

  bool has_se_ = false;
  std::string se_;

  float alpha_ = 1.0;

  // 描画用クラス
  std::unique_ptr<UI::WidgetBase> widget_base_;

  std::vector<WidgetPtr> children_;

  // 画面上のサイズ
  ci::Rectf disp_rect_;

#if defined (DEBUG)
  ci::Color disp_color_;
#endif

};

} }
