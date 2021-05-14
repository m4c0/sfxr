#pragma once

#include "m4c0.ddk.hpp"

#include <cstdint>
#include <map>
#include <thread>

using DWORD = std::uint32_t;
static constexpr const auto DDK_WINDOW = 0;
static constexpr const auto hInstanceMain = 0;
static constexpr const auto hWndMain = 0;

static constexpr const auto DIK_SPACE = ' ';
static constexpr const auto DIK_RETURN = '\n';

static bool mouse_left = false;       // NOLINT
static bool mouse_right = false;      // NOLINT
static bool mouse_leftclick = false;  // NOLINT
static bool mouse_rightclick = false; // NOLINT

class pixel {
  unsigned m_index;

public:
  explicit constexpr pixel(unsigned i) : m_index(i) {
  }

  pixel & operator=(std::uint32_t rgba) {
    write_pixel(m_index, rgba);
    return *this;
  }
};
static class {
public:
  [[nodiscard]] pixel operator[](unsigned idx) noexcept {
    return pixel { idx };
  }
} ddkscreen32; // NOLINT

class DPInput {
public:
  DPInput(int /*hWnd*/, int /*hInst*/) {
  }

  static void Update() {
  }

  static bool KeyPressed(unsigned key) {
    static std::map<unsigned, bool> keys {};

    bool r = keys[key];
    keys[key] = false;
    return r;
  }
};

static bool FileSelectorLoad(unsigned /*x*/, const char * /*name*/, unsigned /*y*/) {
  return false;
}
static bool FileSelectorSave(unsigned /*x*/, const char * /*name*/, unsigned /*y*/) {
  return false;
}

static void Sleep(unsigned ms) {
  using namespace std::chrono_literals;

  std::this_thread::sleep_for(1ms * ms);
}

static void ddkLock() {
  lock();
}
static void ddkUnlock() {
  unlock();
}

static void ddkSetMode(
    int width,
    int height,
    int /*bpp*/,
    int /*refreshrate*/,
    int /*fullscreen*/,
    const char * /*title*/) {
  set_screen_size(width, height);
}
