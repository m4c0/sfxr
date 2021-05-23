#include "m4c0.vulkan.hpp"
#include "m4c0/fuji/main_loop_thread.hpp"
#include "m4c0/native_handles.metal.hpp"
#include "m4c0/objc/ca_metal_layer.hpp"
#include "m4c0/objc/ns_event.hpp"
#include "m4c0/objc/ns_event_type.hpp"
#include "m4c0/objc/ns_window.hpp"
#include "m4c0/osx/main.hpp"

#include <atomic>

class resources : public m4c0::native_handles, public native_stuff {
  m4c0::fuji::main_loop_thread<loop> m_stuff {};
  m4c0::objc::mtk_view m_view;
  std::atomic<float> m_mouse_x {};
  std::atomic<float> m_mouse_y {};

public:
  explicit resources(const m4c0::objc::mtk_view * v) : m_view(*v) {
    m_stuff.start(this, this);
  }
  ~resources() {
    m_stuff.interrupt();
  }

  resources(const resources &) = delete;
  resources(resources &&) = delete;
  resources & operator=(const resources &) = delete;
  resources & operator=(resources &&) = delete;

  [[nodiscard]] CAMetalLayer * layer() const noexcept override {
    return m_view.layer().self();
  }

  void get_mouse_position(float * x, float * y) const override {
    *x = m_mouse_x;
    *y = m_mouse_y;
  }
  void update_mouse(m4c0::objc::cg_point pos) {
    auto bounds = m_view.bounds();
    m_mouse_x = static_cast<float>(pos.x / bounds.size.width);
    m_mouse_y = static_cast<float>((bounds.size.height - pos.y) / bounds.size.height);
  }
};

int main(int argc, char ** argv) {
  static class : public m4c0::osx::delegate {
    std::unique_ptr<resources> r;

  public:
    void start(const m4c0::objc::mtk_view * view) override {
      r = std::make_unique<resources>(view);
    }
    void on_event(const m4c0::objc::ns_event * e) override {
      if (e->type() == m4c0::objc::ns_event_type::mouse_moved) {
        r->update_mouse(e->location_in_window());
      }
    }
    void stop() override {
      r.reset();
    }

  } d;
  return m4c0::osx::main(argc, argv, &d);
}
