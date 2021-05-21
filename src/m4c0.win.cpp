#include "m4c0.vulkan.hpp"
#include "m4c0/fuji/main_loop.hpp"
#include "m4c0/fuji/main_loop_thread.hpp"
#include "m4c0/native_handles.win32.hpp"
#include "m4c0/win/main.hpp"

#include <tchar.h>
#include <windows.h>

LRESULT CALLBACK m4c0::win::window_proc(HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param) {
  static class wnd_provider : public m4c0::native_handles, public native_stuff {
    m4c0::fuji::main_loop_thread<loop> m_stuff {};
    HWND m_hwnd;

  public:
    explicit wnd_provider(HWND hwnd) : m_hwnd(hwnd) {
    }
    [[nodiscard]] HINSTANCE hinstance() const noexcept override {
      return GetModuleHandle(nullptr);
    }
    [[nodiscard]] HWND hwnd() const noexcept override {
      return m_hwnd;
    }

    void start() {
      m_stuff.start(this, this);
    }
    void stop() {
      m_stuff.interrupt();
    }

    void get_mouse_position(float * x, float * y) const override {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(m_hwnd, &p);

      RECT r;
      GetWindowRect(m_hwnd, &r);

      *x = static_cast<float>(p.x) / static_cast<float>(r.right - r.left);
      *y = static_cast<float>(p.y) / static_cast<float>(r.bottom - r.top);
    }
  } wnd { hwnd };

  switch (msg) {
  case WM_CREATE: {
    SetWindowText(hwnd, "SFXR");
    wnd.start();
    return 0;
  }

  case WM_DESTROY:
    wnd.stop();
    PostQuitMessage(0);
    return 0;

  default:
    return DefWindowProc(hwnd, msg, w_param, l_param);
  }
}
