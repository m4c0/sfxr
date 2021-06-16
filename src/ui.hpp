#pragma once

#include "geom.hpp"

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

static constexpr ui_bar bar_item(const box & b, ui_color c) {
  return ui_bar { b, c };
}
static constexpr ui_text text_item(const point & p, ui_color c, const char * txt) {
  return ui_text { p, c, txt };
}
