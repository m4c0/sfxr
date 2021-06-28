#pragma once

#include "m4c0.hpp"
#include "m4c0/assets/simple_asset.hpp"

#include <cstring>
#include <memory>

static constexpr const auto magic_tga_color = 0x300030;

int LoadTGA(const m4c0::native_handles * np, Spriteset & tiles, const char * filename) {
  static constexpr const auto expected_header_size = 18;
  static constexpr const auto pad_size = 11;
  struct header {
    std::uint8_t id_length;
    std::array<std::uint8_t, pad_size> pad;
    std::uint16_t width;
    std::uint16_t height;
    std::uint8_t bits;
    std::uint8_t img_descr;
  };
  static_assert(sizeof(header) == expected_header_size);

  static constexpr const auto expected_pixel_size = 3;
  struct pixel {
    std::uint8_t r, g, b;
  };
  static_assert(sizeof(pixel) == expected_pixel_size);

  auto res = m4c0::assets::simple_asset::load(np, filename, "tga");
  auto h = *(res->span<header>().data());

  tiles.width = h.height;
  tiles.height = h.height;
  tiles.pitch = h.width;

  const auto * data = res->span<pixel>(sizeof(header) + static_cast<std::ptrdiff_t>(h.id_length)).data();
  tiles.data = (DWORD *)malloc(h.width * h.height * sizeof(DWORD));
  for (auto y = h.height - 1; y >= 0; y--) {
    for (auto x = 0; x < h.width; x++) {
      auto r = *data++;
      tiles.data[y * h.width + x] = r.r > 0 ? magic_tga_color : 0;
    }
  }

  return 0;
}

void ClearScreen(DWORD color) {
  draw_bar(0, 0, 640, 480, color);
}

void DrawBar(int sx, int sy, int w, int h, DWORD color) {
  draw_bar(sx, sy, w, h, color);
}

void DrawSprite(Spriteset & sprites, int sx, int sy, int i, DWORD color) {
  for (int y = 0; y < sprites.height; y++) {
    int spoffset = y * sprites.pitch + i * sprites.width;
    for (int x = 0; x < sprites.width; x++) {
      auto p = sprites.data[spoffset++];
      if (p != magic_tga_color) draw_bar(sx + x, sy + y, 1, 1, color);
    }
  }
}

void DrawText(int sx, int sy, DWORD color, const char * string) {
  auto len = strlen(string);
  for (int i = 0; i < len; i++)
    DrawSprite(font, sx + i * 8, sy, string[i] - ' ', color);
}

bool MouseInBox(int x, int y, int w, int h) {
  return mouse_x >= x && mouse_x < x + w && mouse_y >= y && mouse_y < y + h;
}
