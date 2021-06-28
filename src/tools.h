#pragma once

#include "m4c0.hpp"
#include "sprite_set.hpp"

#include <cstring>
#include <memory>

extern sprite_set font;

static void ClearScreen(DWORD color) {
  draw_bar(0, 0, 640, 480, color);
}

static void DrawBar(int sx, int sy, int w, int h, DWORD color) {
  draw_bar(sx, sy, w, h, color);
}

static void DrawText(int sx, int sy, DWORD color, const char * string) {
  auto len = strlen(string);
  for (int i = 0; i < len; i++) {
    font.draw(sx + i * 8, sy, string[i] - ' ', color);
  }
}

static bool MouseInBox(int x, int y, int w, int h) {
  return mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h;
}
