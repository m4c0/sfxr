#pragma once

#include <cstdint>

class draw_context {
public:
  virtual void panel(int sx, int sy, int w, int h, std::uint32_t color) = 0;
};
