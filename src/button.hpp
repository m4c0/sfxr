#pragma once

#include "geom.hpp"
#include "ui.hpp"

struct mouse {
  point pos;
  bool down;
  bool clicked;
};

enum class btn_state {
  normal,
  hover,
  down,
  clicked, // same as down, when the mouse is not down anymore
  highlight,
};

struct btn_colors {
  ui_color c1;
  ui_color c2;
  ui_color c3;
};

using btn_is_hl = bool (*)();
struct button {
  box bounds;
  int id;
  const char * text;
  ui_callback cb;
  btn_is_hl is_highlight;
};

static constexpr auto btn(int x, int y, btn_is_hl highlight, const char * text, int id, ui_callback call) {
  constexpr const auto btn_w = 100;
  constexpr const auto btn_h = 17;

  return button {
    .bounds = box { { x, y }, { x + btn_w, y + btn_h } },
    .id = id,
    .text = text,
    .cb = call,
    .is_highlight = highlight,
  };
}

static constexpr btn_colors btn_colors_for_state(btn_state state) {
  constexpr const ui_color palette0 { 0xA09088 };
  constexpr const ui_color palette1 { 0xFFF0E0 };
  constexpr const ui_color palette2 { 0x000000 };
  constexpr const ui_color palette3 { 0x988070 }; // TODO: do we need a four-color palette?
  switch (state) {
  case btn_state::normal:
    return { palette2, palette0, palette2 };
  case btn_state::hover:
    return { palette2, palette2, palette0 };
  case btn_state::highlight:
    return { palette2, palette3, palette1 };
  case btn_state::clicked: // TODO: different color, maybe?
  case btn_state::down:
    return { palette0, palette1, palette0 };
  default:
    std::terminate(); // TODO: find a better way to force compiler to ignore this branch
  };
}
static constexpr bool is_button_clicked(btn_state state) {
  return state == btn_state::clicked;
}
static constexpr bool is_button_down(btn_state state) {
  return state == btn_state::down;
}

static constexpr btn_state imm_button_state(const mouse & mouse, const button & btn, int cur_btn_id) {
  bool hover = in_box(mouse.pos, btn.bounds);
  bool pressing = hover && mouse.clicked;
  bool current = pressing || (cur_btn_id == btn.id);
  if (current && hover && mouse.down) {
    return btn_state::down;
  }
  if (current && hover) {
    return btn_state::clicked;
  }
  if (hover) {
    return btn_state::hover;
  }
  if (btn.is_highlight()) {
    return btn_state::highlight;
  }
  return btn_state::normal;
}

static constexpr const auto btn_margin = 5;
static constexpr auto btn_ui_bar_bg(const button & btn, const btn_colors & colors) {
  return bar_item(extend(btn.bounds, 1), colors.c1);
}
static constexpr auto btn_ui_bar_fg(const button & btn, const btn_colors & colors) {
  return bar_item(btn.bounds, colors.c2);
}
static constexpr auto btn_ui_text(const button & btn, const btn_colors & colors) {
  return text_item(add(btn.bounds.p1, btn_margin), colors.c3, btn.text);
}
static constexpr auto btn_ui_result(const button & btn, btn_state state) {
  return ui_result {
    .sel = cond(is_button_down(state), btn.id),
    .clicked = cond(is_button_clicked(state), btn.cb),
  };
}
