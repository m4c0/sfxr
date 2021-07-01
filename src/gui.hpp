#pragma once

#include "draw_context.hpp"
#include "geom.hpp"
#include "mouse.hpp"
#include "sprite_set.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

static constexpr const auto bg_color = 0xC0B090;
static constexpr const auto txt_color = 0x504030;
static constexpr const auto bar_color = 0x000000;

namespace gui {
  class widget { // NOLINT
  public:
    virtual ~widget() = default;

    virtual void draw(draw_context * ctx, const rect & r) const = 0;

    [[nodiscard]] virtual size min_size() const = 0;
  };

  class label : public widget {
    static constexpr const auto chr_w = 8;

    std::string_view str;

  public:
    explicit constexpr label(const char * str) : str(str) {
    }

    void draw(draw_context * ctx, const rect & r) const override {
      ctx->text(r.origin, str, txt_color);
    }
    [[nodiscard]] size min_size() const override {
      return { static_cast<int>(str.length() * chr_w), chr_w };
    }
  };

  class panel : public widget {
    std::uint32_t color;

  public:
    explicit constexpr panel(std::uint32_t color) : color(color) {
    }

    void draw(draw_context * ctx, const rect & r) const override {
      ctx->panel(r, color);
    }
    [[nodiscard]] size min_size() const override {
      return {};
    }
  };

  class button : public widget {
    static constexpr const auto palette = std::array { 0x000000, 0xA09088, 0x988070, 0xFFF0E0 };
    static constexpr const auto pal_normal = std::array { palette[0], palette[1], palette[0] };
    static constexpr const auto pal_highlight = std::array { palette[0], palette[2], palette[3] };
    static constexpr const auto pal_down = std::array { palette[1], palette[3], palette[1] };
    static constexpr const auto pal_hover = std::array { palette[0], palette[0], palette[1] };

    static constexpr const auto width = 100;
    static constexpr const auto height = 18;
    static constexpr const auto font_height = 8;
    static constexpr const auto label_margin = (height - font_height) / 2;
    static constexpr const auto min_sz = size { width, height };

    unsigned id;
    const char * label;
    void (*callback)();
    bool highlight;

    [[nodiscard]] constexpr auto colors(bool hover, bool current) const {
      if (current && hover) {
        return pal_down;
      }
      if (hover) {
        return pal_hover;
      }
      if (highlight) {
        return pal_highlight;
      }
      return pal_normal;
    }

  public:
    constexpr button(unsigned id, const char * label, void (*callback)(), bool highlight = false)
      : id(id)
      , label(label)
      , callback(callback)
      , highlight(highlight) {
    }

    void draw(draw_context * ctx, const rect & r) const override {
      auto * mouse = ctx->mouse();
      bool hover = mouse->is_in_box(r);
      if (hover && mouse->is_left_down_edge()) mouse->holder() = id;
      bool current = mouse->holder() == id;

      if (current && hover && !mouse->is_left_down()) callback();

      auto c = colors(hover, current);
      ctx->panel({ r.origin, min_sz }, c[0]);
      ctx->panel({ r.origin + 1, min_sz - 2 }, c[1]);
      ctx->text(r.origin + label_margin, label, c[2]);
    }
    [[nodiscard]] size min_size() const override {
      return min_sz;
    }
  };

  class slider : public widget {
    static constexpr const auto sensitivity = 0.005F;
    static constexpr const auto palette = std::array { 0x000000, 0xF0C090, 0x807060, 0xFFFFFF };
    static constexpr const auto min_sz = size { 100, 10 };

    unsigned id;
    float * value;
    float min, max;

  public:
    constexpr slider(unsigned id, float * value, float min, float max) : id(id), value(value), min(min), max(max) {
    }
    void draw(draw_context * ctx, const rect & r) const override {
      auto * mouse = ctx->mouse();
      bool hover = mouse->is_in_box(r);
      if (hover && mouse->is_left_down_edge()) mouse->holder() = id;
      bool current = mouse->holder() == id;

      auto mv = current ? static_cast<float>(mouse->delta().x) : 0.0F;
      *value = std::clamp(*value + mv * sensitivity, min, max);

      auto ir = r - 1;
      auto ival = static_cast<int>(static_cast<float>(ir.size.w) * (*value - min) / (max - min));

      ctx->panel(r, palette[0]);
      ctx->panel(ir, hover ? palette[0] : palette[2]);
      ctx->panel({ ir.origin, { ival, ir.size.h } }, hover ? palette[3] : palette[1]);
      ctx->panel({ ir.origin + point { ival, 0 }, { 1, ir.size.h } }, palette[3]);
    }
    [[nodiscard]] size min_size() const override {
      return min_sz;
    }
  };

  class box {
    std::vector<std::unique_ptr<widget>> m_children;
    int m_margin = 4;

  protected:
    [[nodiscard]] constexpr const auto & children() const noexcept {
      return m_children;
    }
    [[nodiscard]] constexpr auto margin() const noexcept {
      return m_margin;
    }

  public:
    explicit box(int margin = 4) : m_margin(margin) {
    }

    template<class Tp, class... Args>
    auto make_child(Args &&... args) -> std::enable_if_t<std::is_base_of_v<widget, Tp>, Tp *> {
      auto up = std::make_unique<Tp>(std::forward<Args>(args)...);
      auto * res = up.get();
      m_children.emplace_back(std::move(up));
      return res;
    }
  };

  class hbox : public box, public widget {
  public:
    using box::box;

    void draw(draw_context * ctx, const rect & r) const override {
      auto p = r.origin;
      for (const auto & c : children()) {
        auto ms = c->min_size();
        c->draw(ctx, rect { p, ms });
        p.x += ms.w + margin();
      }
    }

    [[nodiscard]] size min_size() const override {
      const size init { static_cast<int>(children().size()) * margin(), 0 };
      return std::accumulate(children().begin(), children().end(), init, [](size res, auto & w) {
        auto ms = w->min_size();
        return size { res.w + ms.w, std::max(res.h, ms.h) };
      });
    }
  };

  class vbox : public box, public widget {
  public:
    using box::box;

    void draw(draw_context * ctx, const rect & r) const override {
      auto p = r.origin;
      for (const auto & c : children()) {
        auto ms = c->min_size();
        c->draw(ctx, rect { p, ms });
        p.y += ms.h + margin();
      }
    }

    [[nodiscard]] size min_size() const override {
      const size init { 0, static_cast<int>(children().size()) * margin() };
      return std::accumulate(children().begin(), children().end(), init, [](size res, auto & w) {
        auto ms = w->min_size();
        return size { std::max(res.w, ms.w), res.h + ms.h };
      });
    }
  };

  class stack_box : public box, public widget {
  public:
    using box::box;

    void draw(draw_context * ctx, const rect & r) const override {
      const auto rm = r - margin();
      for (const auto & c : children()) {
        c->draw(ctx, rm);
      }
    }

    [[nodiscard]] size min_size() const override {
      constexpr const size init { 0, 0 };
      const auto res = std::accumulate(children().begin(), children().end(), init, [](size res, auto & w) {
        auto ms = w->min_size();
        return size { std::max(res.w, ms.w), std::max(res.h, ms.h) };
      });
      return res + margin() * 2;
    }
  };
}
