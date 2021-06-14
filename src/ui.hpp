#pragma once

#include "geom.hpp"
#include "utils.hpp"

#include <array>
#include <optional>

using ui_color = unsigned;

struct ui_bar {
  box bounds;
  ui_color color;
};
struct ui_text {
  point pos;
  ui_color color;
  const char * text;
};

using ui_callback = void (*)();

struct ui_result {
  std::optional<int> sel;
  std::optional<ui_callback> clicked;
};

static constexpr ui_result operator+(const ui_result & a, const ui_result b) {
  return ui_result {
    .sel = opt_chain(a.sel, b.sel),
    .clicked = opt_chain(a.clicked, b.clicked),
  };
}

static constexpr ui_bar bar_item(const box & b, ui_color c) {
  return ui_bar { b, c };
}
static constexpr ui_text text_item(const point & p, ui_color c, const char * txt) {
  return ui_text { p, c, txt };
}
