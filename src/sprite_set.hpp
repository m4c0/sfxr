#pragma once

#include "m4c0.ddk.hpp"
#include "m4c0/assets/simple_asset.hpp"
#include "m4c0/native_handles.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

struct sprite_set {
  static constexpr const auto magic_tga_color = 0x300030;
  using pixel_t = std::uint32_t;

  std::vector<pixel_t> data {};
  int width {};
  int height {};
  int pitch {};

  static sprite_set load(const m4c0::native_handles * np, const char * filename, bool has_tiles) {
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

    sprite_set tiles;
    tiles.width = has_tiles ? h.height : h.width;
    tiles.height = h.height;
    tiles.pitch = h.width;
    tiles.data.reserve(h.width * h.height * sizeof(pixel_t));

    const auto data = res->span<pixel>(sizeof(header) + static_cast<std::ptrdiff_t>(h.id_length));
    for (auto p = 0, y = h.height - 1; y >= 0; y--) {
      for (auto x = 0; x < h.width; x++, p++) {
        auto r = data[p];
        tiles.data[y * h.width + x] = r.r > 0 ? magic_tga_color : 0;
      }
    }

    return tiles;
  }

  void draw(int sx, int sy, int i, pixel_t color) const {
    for (int y = 0; y < height; y++) {
      int spoffset = y * pitch + i * width;
      for (int x = 0; x < width; x++) {
        auto p = data[spoffset++];
        if (p != magic_tga_color) draw_bar(sx + x, sy + y, 1, 1, color);
      }
    }
  }
};
