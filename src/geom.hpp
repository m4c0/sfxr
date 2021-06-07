#pragma once

struct point {
  int x;
  int y;
};
struct box {
  point p1;
  point p2;
};

static constexpr bool operator>=(const point & a, const point & b) noexcept {
  return a.x >= b.x && a.y >= b.y;
}
static constexpr bool operator<=(const point & a, const point & b) noexcept {
  return a.x <= b.x && a.y <= b.y;
}
static constexpr point add(const point & p, int dx) {
  return point { p.x + dx, p.y + dx };
}

static constexpr bool in_box(const point & p, const box & b) {
  return b.p1 <= p && p <= b.p2;
}
static constexpr box extend(const box & b, int dx) {
  return box { add(b.p1, -dx), add(b.p2, dx) };
}
