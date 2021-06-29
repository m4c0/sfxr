#pragma once

#include "m4c0/native_handles.hpp"

#include <cstdint>
#include <vector>

struct sprite_set {
  using pixel_t = std::uint32_t;

  std::vector<pixel_t> data {};
  int width {};
  int height {};
  int pitch {};

  static sprite_set load(const m4c0::native_handles * np, const char * filename, bool has_tiles);

  void draw(int sx, int sy, int i, pixel_t color) const;
};
