#include "m4c0.ddk.hpp"

unsigned ddkpitch; // NOLINT

std::function<void()> lock;   // NOLINT
std::function<void()> unlock; // NOLINT

std::function<void(int, int)> set_screen_size;            // NOLINT
std::function<void(unsigned, std::uint32_t)> write_pixel; // NOLINT
