#pragma once

#include "draw_context.hpp"

static constexpr const auto bg_color = 0xC0B090;
static constexpr const auto txt_color = 0x504030;
static constexpr const auto bar_color = 0x000000;

namespace gui {
  class panel {
    int sx, sy, w, h;
    std::uint32_t color;

  public:
    constexpr panel(int sx, int sy, int w, int h, std::uint32_t color) : sx(sx), sy(sy), w(w), h(h), color(color) {
    }

    void draw(draw_context * ctx) const {
      ctx->panel(sx, sy, w, h, color);
    }
  };
  class screen {
    const panel bg { 0, 0, 640, 480, bg_color };

  public:
    void draw(draw_context * ctx) const {
      bg.draw(ctx);
    }
  };
}
