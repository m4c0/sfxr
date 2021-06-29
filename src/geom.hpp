#pragma once

namespace gui {
  struct point {
    int x, y;
  };
  struct size {
    int w, h;
  };
  struct rect {
    point origin;
    size size;
  };

  [[nodiscard]] constexpr point operator+(point a, int b) {
    return { a.x + b, a.y + b };
  }
  [[nodiscard]] constexpr point operator-(point a, int b) {
    return { a.x - b, a.y - b };
  }
  [[nodiscard]] constexpr point operator+(point a, point b) {
    return { a.x + b.x, a.y + b.y };
  }
  [[nodiscard]] constexpr point operator-(point a, point b) {
    return { a.x - b.x, a.y - b.y };
  }

  [[nodiscard]] constexpr size operator+(size a, int b) {
    return { a.w + b, a.h + b };
  }
  [[nodiscard]] constexpr size operator-(size a, int b) {
    return { a.w - b, a.h - b };
  }

  [[nodiscard]] constexpr rect operator+(rect a, int b) {
    return { a.origin - b, a.size + 2 * b };
  }
  [[nodiscard]] constexpr rect operator-(rect a, int b) {
    return { a.origin + b, a.size - 2 * b };
  }

  [[nodiscard]] constexpr bool is_point_in_rect(point p, const rect & r) {
    return p.x >= r.origin.x && p.x < (r.origin.x + r.size.w) && p.y >= r.origin.y && p.y < (r.origin.y + r.size.h);
  }
}
