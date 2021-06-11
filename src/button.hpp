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
  case btn_state::down:
    return { palette0, palette1, palette0 };
  };
}
static constexpr bool is_button_clicked(btn_state state, bool mouse_up) {
  return state == btn_state::down && mouse_up;
}

static constexpr btn_state imm_button_state(const mouse & mouse, const button & btn, int cur_btn_id) {
  bool hover = in_box(mouse.pos, btn.bounds);
  bool pressing = hover && mouse.clicked;
  bool current = pressing || (cur_btn_id == btn.id);
  if (current && hover) {
    return btn_state::down;
  }
  if (hover) {
    return btn_state::hover;
  }
  if (btn.is_highlight()) {
    return btn_state::highlight;
  }
  return btn_state::normal;
}

static constexpr auto btn_ui_items(const button & btn, btn_state state) {
  constexpr const auto btn_margin = 5;
  auto colors = btn_colors_for_state(state);
  return std::array {
    bar_item(extend(btn.bounds, 1), colors.c1),
    bar_item(btn.bounds, colors.c2),
    text_item(add(btn.bounds.p1, btn_margin), colors.c3, btn.text),
  };
}

static constexpr ui_result<3> imm_button(const mouse & ms, const button & btn, int cur_btn) {
  auto state = imm_button_state(ms, btn, cur_btn);
  return ui_result<3> {
    .items = btn_ui_items(btn, state),
    .sel = cond(state == btn_state::down, btn.id),
    .clicked = cond(is_button_clicked(state, !ms.down), btn.cb),
  };
}
