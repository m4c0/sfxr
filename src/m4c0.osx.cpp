#include "m4c0.vulkan.hpp"
#include "m4c0/fuji/main_loop_thread.hpp"
#include "m4c0/native_handles.metal.hpp"
#include "m4c0/objc/ca_metal_layer.hpp"
#include "m4c0/objc/ns_window.hpp"
#include "m4c0/osx/main.hpp"

int main(int argc, char ** argv) {
  static class : public m4c0::osx::delegate, public m4c0::native_handles, public native_stuff {
    m4c0::fuji::main_loop_thread<loop> m_stuff {};
    m4c0::objc::mtk_view m_view {};

  public:
    void start(const m4c0::objc::mtk_view * view) override {
      m_view = *view;
      m_stuff.start(this, this);
    }
    void stop() override {
      m_stuff.interrupt();
    }

    [[nodiscard]] CAMetalLayer * layer() const noexcept override {
      return m_view.layer().self();
    }

    void get_mouse_position(float * x, float * y) const override {
      auto pos = m_view.window().mouse_location_outside_of_event_stream();
      auto bounds = m_view.bounds();
      *x = pos.x / bounds.size.width;
      *y = (bounds.size.height - pos.y) / bounds.size.height;
    }
  } d;
  return m4c0::osx::main(argc, argv, &d);
}
