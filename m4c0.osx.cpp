#include "m4c0.vulkan.hpp"
#include "m4c0/fuji/main_loop_thread.hpp"
#include "m4c0/objc/casts.hpp"
#include "m4c0/osx/main.hpp"
#include "m4c0/vulkan/surface.metal.hpp"

int main(int argc, char ** argv) {
  static class : public m4c0::osx::delegate, public m4c0::vulkan::native_provider {
    m4c0::fuji::main_loop_thread<m4c0::fuji::main_loop_with_stuff<stuff>> m_stuff {};
    CAMetalLayer * m_layer {};

  public:
    void start(void * view) override {
      m_layer = m4c0::objc::objc_msg_send<CAMetalLayer *>(view, "layer");
      m_stuff.start("SFXR", this);
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
