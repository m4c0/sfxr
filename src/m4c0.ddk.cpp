#include "m4c0.ddk.hpp"

unsigned ddkpitch; // NOLINT

std::atomic_int mouse_x;  // NOLINT
std::atomic_int mouse_y;  // NOLINT
std::atomic_int mouse_px; // NOLINT
std::atomic_int mouse_py; // NOLINT

std::function<void()> lock;   // NOLINT
std::function<void()> unlock; // NOLINT

std::function<void(int, int)> set_screen_size;            // NOLINT
std::function<void(unsigned, std::uint32_t)> write_pixel; // NOLINT
