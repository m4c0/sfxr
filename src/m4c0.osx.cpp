#include "m4c0.vulkan.hpp"
#include "m4c0/casein/main.osx.hpp"
#include "m4c0/native_handles.metal.hpp"
#include "m4c0/objc/ca_metal_layer.hpp"
#include "m4c0/objc/ns_event.hpp"
#include "m4c0/objc/ns_event_type.hpp"
#include "m4c0/objc/ns_window.hpp"
#include "m4c0/osx/main.hpp"

#include <atomic>

static class : public native_stuff {
public:
  void update_mouse(m4c0::objc::cg_rect bounds, m4c0::objc::cg_point pos) {
    auto x = static_cast<float>(pos.x / bounds.size.width);
    auto y = static_cast<float>((bounds.size.height - pos.y) / bounds.size.height);
    update_mouse_pos(x, y);
  }
  using native_stuff::update_mouse_down;
} natives; // NOLINT
const native_stuff * const natives_ptr = &natives;

class my_main : public m4c0::casein::osx::main {
public:
  void on_event(const m4c0::objc::ns_event * e) override {
    switch (e->type()) {
    case m4c0::objc::ns_event_type::mouse_moved:
    case m4c0::objc::ns_event_type::mouse_left_dragged:
      natives.update_mouse(view().bounds(), e->location_in_window());
      break;
    case m4c0::objc::ns_event_type::mouse_left_down:
      natives.update_mouse_down(true);
      break;
    case m4c0::objc::ns_event_type::mouse_left_up:
      natives.update_mouse_down(false);
      break;
    default:
      break;
    }
  }
};

int main(int argc, char ** argv) {
  my_main d;
  return m4c0::osx::main(argc, argv, &d);
}
