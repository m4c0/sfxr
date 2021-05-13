#include "m4c0.vulkan.hpp"
#include "m4c0/fuji/main_loop_thread.hpp"
#include "m4c0/native_handles.metal.hpp"
#include "m4c0/objc/casts.hpp"
#include "m4c0/osx/main.hpp"

int main(int argc, char ** argv) {
  static class : public m4c0::osx::delegate, public m4c0::native_handles {
    m4c0::fuji::main_loop_thread<loop> m_stuff {};
    CAMetalLayer * m_layer {};
    void * m_window {};

  public:
    void start(void * view) override {
      m_window = m4c0::objc::objc_msg_send<void *>(view, "window");
      m_layer = m4c0::objc::objc_msg_send<CAMetalLayer *>(view, "layer");
      m_stuff.start(this);
    }
    void stop() override {
      m_stuff.interrupt();
    }

    [[nodiscard]] CAMetalLayer * layer() const noexcept override {
      return m_layer;
    }
  } d;
  return m4c0::osx::main(argc, argv, &d);
}
