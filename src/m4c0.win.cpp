#include "m4c0.vulkan.hpp"
#include "m4c0/casein/main.hpp"
#include "m4c0/casein/main.win.hpp"
#include "m4c0/native_handles.win32.hpp"
#include "m4c0/win/main.hpp"

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>

static class : public native_stuff {
public:
  void update_mouse(HWND hwnd, LPARAM l) {
    auto px = static_cast<float>(GET_X_LPARAM(l)); // NOLINT
    auto py = static_cast<float>(GET_Y_LPARAM(l)); // NOLINT

    RECT r;
    GetClientRect(hwnd, &r);

    float x = px / static_cast<float>(r.right - r.left);
    float y = py / static_cast<float>(r.bottom - r.top);
    update_mouse_pos(x, y);
  }
  using native_stuff::update_mouse_down;
} natives; // NOLINT
const native_stuff * const natives_ptr = &natives;

LRESULT CALLBACK m4c0::win::window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  switch (msg) {
  case WM_MOUSEMOVE:
    natives.update_mouse(hwnd, l_param);
    return 0;

  case WM_LBUTTONDOWN:
    natives.update_mouse(hwnd, l_param);
    natives.update_mouse_down(true);
    return 0;

  case WM_LBUTTONUP:
    natives.update_mouse(hwnd, l_param);
    natives.update_mouse_down(false);
    return 0;

  default:
    return m4c0::casein::win::window_proc(hwnd, msg, w_param, l_param);
  }
}
