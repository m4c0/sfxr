#pragma once

#include <cstdint>
#include <functional>

void ddkInit();
bool ddkCalcFrame();
void ddkFree();

// Stuff that is called from the "sfxr" thread
// They are way too dynamic to allow integrating with DrPetter's code without changing it

extern unsigned ddkpitch; // NOLINT

extern std::function<void()> lock;   // NOLINT
extern std::function<void()> unlock; // NOLINT

extern std::function<void(int, int)> set_screen_size;            // NOLINT
extern std::function<void(unsigned, std::uint32_t)> write_pixel; // NOLINT
