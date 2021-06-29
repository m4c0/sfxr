#pragma once

#include "geom.hpp"

namespace gui {
  class mouse {
    unsigned m_holder {};

  public:
    [[nodiscard]] constexpr auto & holder() noexcept {
      return m_holder;
    }

    [[nodiscard]] virtual bool is_left_down() = 0;
    [[nodiscard]] virtual bool is_left_down_edge() = 0;
    [[nodiscard]] virtual point delta() = 0;
    [[nodiscard]] virtual point position() = 0;

    [[nodiscard]] bool is_in_box(const rect & r) {
      return is_point_in_rect(position(), r);
    }
  };
}
