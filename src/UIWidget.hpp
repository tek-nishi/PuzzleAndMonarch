#pragma once

//
// UI Widget
//

#include "UIText.hpp"
#include "UIRoundRect.hpp"


namespace ngs { namespace UI {

// TIPS:自分自身を引数に取る関数があるので先行宣言が必要
class Widget;
using WidgetPtr = std::shared_ptr<Widget>;


class Widget {

  
public:
  Widget(std::string identifier, const ci::Rectf& rect) noexcept
    : identifier_(std::move(identifier)),
      rect_(rect)
  {
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

  void setColor(const ci::ColorA& color) {
    color_ = color;
  }


  void setText(std::string text) noexcept
  {
    text_ = std::move(text);
  }

  const std::string& getText() const noexcept
  {
    return text_;
  }

  void setTextSize(const float text_size) noexcept
  {
    text_size_ = text_size;
  }

  void setAlignment(const glm::vec2& alignment) noexcept
  {
    alignment_ = alignment;
  }
  
  void addChild(const WidgetPtr& widget) noexcept
  {
    children_.push_back(widget);
  }

  const std::vector<WidgetPtr>& getChildren() const noexcept
  {
    return children_;
  }


  void draw(const ci::Rectf& parent_rect, const glm::vec2& parent_scale,
            UI::Drawer& drawer) const noexcept
  {
    // とりあえず描く
    auto rect = calcRect(parent_rect, parent_scale);

    ci::gl::color(color_);
    ci::gl::drawStrokedRect(rect);

    if (!text_.empty())
    {
      ci::gl::pushModelMatrix();
      ci::gl::ScopedGlslProg prog(drawer.getFontShader());

      auto& font = drawer.getFont();
      auto size = font.drawSize(text_);
      ci::gl::translate(rect.getCenter() - size * alignment_ * text_size_);
      ci::gl::scale(glm::vec3(text_size_));
      font.draw(text_, glm::vec2(0, 0), color_);
      ci::gl::popModelMatrix();
    }
    
    for (const auto& child : children_)
    {
      auto scale = parent_scale * scale_;
      child->draw(rect, scale, drawer);
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

  ci::Rectf rect_;

  // スケーリングの中心(normalized)
  glm::vec2 pivot_ = { 0.5f, 0.5f };

  // 親のサイズの影響力(normalized)
  glm::vec2 anchor_min_ = { 0.5f, 0.5f };
  glm::vec2 anchor_max_ = { 0.5f, 0.5f };

  glm::vec2 scale_ = { 1.0f, 1.0f };

  ci::ColorA color_ = { 1.0f, 1.0f, 1.0f, 1.0f };

  std::string text_;
  float text_size_ = 1.0f;
  glm::vec2 alignment_ = { 0.5, 0.5 };


  std::vector<WidgetPtr> children_;

};

} }
