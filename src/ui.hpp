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
enum class ui_item_type { bar, text };
struct ui_item { // NOLINT
  union {
    ui_bar bar;
    ui_text text;
  } data;
  ui_item_type type;
};

using ui_callback = void (*)();

template<unsigned Qty>
struct ui_result {
  std::array<ui_item, Qty> items;
  std::optional<int> sel;
  std::optional<ui_callback> clicked;
};

template<unsigned A, unsigned B>
static constexpr ui_result<A + B> operator+(const ui_result<A> & a, const ui_result<B> b) {
  return ui_result<A + B> {
    .items = concat(a.items, b.items),
    .sel = opt_chain(a.sel, b.sel),
    .clicked = opt_chain(a.clicked, b.clicked),
  };
}

static constexpr ui_item bar_item(const box & b, ui_color c) {
  return ui_item {
    .data = { .bar = ui_bar { b, c } },
    .type = ui_item_type::bar,
  };
}
static constexpr ui_item text_item(const point & p, ui_color c, const char * txt) {
  return ui_item {
    .data = { .text = ui_text { p, c, txt } },
    .type = ui_item_type::text,
  };
}
