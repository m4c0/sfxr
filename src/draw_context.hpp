#pragma once

#include "geom.hpp"
#include "sprite_set.hpp"

#include <cstdint>
#include <string_view>

namespace gui {
  class mouse;

  class draw_context {
  protected:
    [[nodiscard]] virtual sprite_set * font() = 0;

  public:
    static constexpr const auto chr_w = 8;

    virtual mouse * mouse() = 0;

    virtual void panel(const gui::rect & r, std::uint32_t color) = 0;

    void text(gui::point p, std::string_view str, std::uint32_t color) {
      auto * fnt = font();

      auto x = p.x;
      for (auto c : str) {
        fnt->draw(x, p.y, c - ' ', color);
        x += chr_w;
      }
    }
  };
}
