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
  Widget(std::string identifier, const ci::Rectf& rect) noexcept
    : identifier_(std::move(identifier)),
      rect_(rect)
  {
#if defined (DEBUG)
    // 枠線のために適当に色を決める
    disp_color_ = ci::Color8u(ci::randInt(256),
                              ci::randInt(256),
                              ci::randInt(256));
#endif
  }


  const std::string& getIdentifier() const noexcept
  {
    return identifier_;
  }


  ci::Rectf& getRect() noexcept
  {
    return rect_;
  }
  
  void setPivot(const glm::vec2& pivot) noexcept
  {
    pivot_ = pivot;
  }

  // for Editor
  glm::vec2& getPivot() noexcept
  {
    return pivot_;
  }
  
  void setAnchor(const glm::vec2& anchor_min, const glm::vec2& anchor_max) noexcept
  {
    anchor_min_ = anchor_min;
    anchor_max_ = anchor_max;
  }

  // for Editor
  glm::vec2& getAnchorMin() noexcept
  {
    return anchor_min_;
  }

  // for Editor
  glm::vec2& getAnchorMax() noexcept
  {
    return anchor_min_;
  }

  void setScale(const glm::vec2& scale) noexcept
  {
    scale_ = scale;
  }

  // for Editor
  glm::vec2& getScale() noexcept
  {
    return scale_;
  }

  void setEvent(std::string name) noexcept
  {
    event_ = std::move(name);
    has_event_ = true;
  }

  const std::string& getEvent() const noexcept
  {
    return event_;
  }


  void addChild(const WidgetPtr& widget) noexcept
  {
    children_.push_back(widget);
  }

  const std::vector<WidgetPtr>& getChildren() const noexcept
  {
    return children_;
  }

  void setWidgetBase(std::unique_ptr<UI::WidgetBase> base) noexcept
  {
    widget_base_ = std::move(base);
  }

  bool hasEvent() const noexcept
  {
    return has_event_;
  }

  // FIXME 直前の描画結果から判定している
  bool contains(const glm::vec2& point) const noexcept
  {
    return disp_rect_.contains(point);
  }

  // 画面に表示するかどうかの制御
  //   全ての子供にも影響
  void enable(const bool enable = true) noexcept
  {
    enable_ = enable;
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
                } },
      { "pivot", [this](const boost::any& v) noexcept
                 {
                   pivot_ = boost::any_cast<const glm::vec2&>(v);
                 } },
      { "anchor_min", [this](const boost::any& v) noexcept
                      {
                        anchor_min_ = boost::any_cast<const glm::vec2&>(v);
                      } },
      { "anchor_max", [this](const boost::any& v) noexcept
                      {
                        anchor_max_ = boost::any_cast<const glm::vec2&>(v);
                      } },
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


  void draw(const ci::Rectf& parent_rect, const glm::vec2& parent_scale,
            UI::Drawer& drawer) noexcept
  {
    if (!enable_) return;

    disp_rect_ = calcRect(parent_rect, parent_scale);
    widget_base_->draw(disp_rect_, drawer);

#if defined (DEBUG)
    // デバッグ用にRectを描画
    ci::gl::color(disp_color_);
    ci::gl::drawStrokedRect(disp_rect_);
#endif
    
    for (const auto& child : children_)
    {
      auto scale = parent_scale * scale_;
      child->draw(disp_rect_, scale, drawer);
    }
  }


private:
  // 親の情報から自分の位置、サイズを計算
  ci::Rectf calcRect(const ci::Rectf& parent_rect, const glm::vec2& scale) const noexcept
  {
    glm::vec2 parent_size = parent_rect.getSize();

    // 親のサイズとアンカーから左下・右上の座標を計算
    glm::vec2 anchor_min = parent_size * anchor_min_;
    glm::vec2 anchor_max = parent_size * anchor_max_;

    // 相対座標(スケーリング抜き)
    glm::vec2 pos  = rect_.getUpperLeft()  + anchor_min;
    glm::vec2 size = rect_.getLowerRight() + anchor_max - pos;

    // pivotを考慮したスケーリング
    glm::vec2 d = size * pivot_;
    pos -= d * scale - d;
    size *= scale;

    glm::vec2 parent_pos = parent_rect.getUpperLeft();
    return ci::Rectf(pos + parent_pos, pos + size + parent_pos);
  }


  // メンバ変数
  std::string identifier_;
  bool enable_ = true;

  ci::Rectf rect_;

  // スケーリングの中心(normalized)
  glm::vec2 pivot_ = { 0.5f, 0.5f };

  // 親のサイズの影響力(normalized)
  glm::vec2 anchor_min_ = { 0.5f, 0.5f };
  glm::vec2 anchor_max_ = { 0.5f, 0.5f };

  glm::vec2 scale_ = { 1.0f, 1.0f };

  bool has_event_ = false;
  std::string event_;

  // 描画用クラス
  std::unique_ptr<UI::WidgetBase> widget_base_;

  std::vector<WidgetPtr> children_;

  // 画面上のサイズ
  ci::Rectf disp_rect_;

#if defined (DEBUG)
  ci::Color8u disp_color_;
#endif

};

} }
