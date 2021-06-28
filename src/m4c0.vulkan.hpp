#pragma once

#include "m4c0.ddk.hpp"
#include "m4c0/casein/main.hpp"
#include "m4c0/fuji/device_context.hpp"
#include "m4c0/fuji/main_loop.hpp"
#include "m4c0/native_handles.hpp"
#include "m4c0/vulkan/bind_pipeline.hpp"
#include "m4c0/vulkan/bind_vertex_buffer.hpp"
#include "m4c0/vulkan/buffer.hpp"
#include "m4c0/vulkan/buffer_memory_bind.hpp"
#include "m4c0/vulkan/device_memory.hpp"
#include "m4c0/vulkan/draw.hpp"
#include "m4c0/vulkan/extent_2d.hpp"
#include "m4c0/vulkan/full_extent_viewport.hpp"
#include "m4c0/vulkan/pipeline.hpp"
#include "m4c0/vulkan/pipeline_layout.hpp"
#include "m4c0/vulkan/push_constants.hpp"
#include "m4c0/vulkan/shader_module.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <thread>

template<class DataTp>
static auto shader(DataTp && data, unsigned size) {
  const auto * void_data = static_cast<const void *>(data);
  auto spv = std::span<const std::uint32_t>(static_cast<const std::uint32_t *>(void_data), size);
  return m4c0::vulkan::shader_module::create_from_spv(spv);
}

class vtx_mem {
  using buffer = m4c0::vulkan::buffer;
  using memory = m4c0::vulkan::device_memory;
  using bind = m4c0::vulkan::tools::buffer_memory_bind;

  buffer m_buffer;
  memory m_memory;
  bind m_bind;

  struct pos {
    float x;
    float y;
  };

  static constexpr const auto verts =
      std::array { pos { 0, 0 }, pos { 0, 1 }, pos { 1, 0 }, pos { 1, 0 }, pos { 0, 1 }, pos { 1, 1 } };

public:
  explicit vtx_mem(const m4c0::fuji::device_context * ld)
    : m_buffer(buffer::create_vertex_buffer_with_size(sizeof(verts))) // NOLINT
    , m_memory(ld->create_host_memory(&m_buffer))
    , m_bind(&m_buffer, &m_memory) {
    auto map = m_memory.map_all();

    std::copy(verts.begin(), verts.end(), map.pointer<pos>());
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) {
    m4c0::vulkan::cmd::bind_vertex_buffer(cb).with_buffer(&m_buffer).with_first_index(0).now();
  }

  [[nodiscard]] static constexpr auto count() noexcept {
    return verts.size();
  }
  [[nodiscard]] static constexpr auto stride() noexcept {
    return sizeof(pos);
  }
};

struct instance {
  std::int16_t x, y, z, w;
  std::uint32_t rgba;
};
static_assert(sizeof(instance) == 3 * sizeof(std::uint32_t));

class color_mem {
  using buffer = m4c0::vulkan::buffer;
  using memory = m4c0::vulkan::device_memory;
  using bind = m4c0::vulkan::tools::buffer_memory_bind;

  unsigned m_count;
  buffer m_buffer;
  memory m_memory;
  bind m_bind;

public:
  color_mem(const m4c0::fuji::device_context * ld, std::uint16_t w, std::uint16_t h)
    : m_count(w * h)
    , m_buffer(buffer::create_vertex_buffer_with_size(m_count * sizeof(instance)))
    , m_memory(ld->create_host_memory(&m_buffer))
    , m_bind(&m_buffer, &m_memory) {
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) const {
    m4c0::vulkan::cmd::bind_vertex_buffer(cb).with_buffer(&m_buffer).with_first_index(1).now();
  }

  [[nodiscard]] constexpr auto * device_memory() noexcept {
    return &m_memory;
  }

  [[nodiscard]] constexpr auto count() const noexcept {
    return m_count;
  }
  [[nodiscard]] static constexpr auto stride() noexcept {
    return sizeof(instance);
  }
};

class pipeline {
  using pipe = m4c0::vulkan::pipeline;
  using pipeline_layout = m4c0::vulkan::pipeline_layout;

  pipeline_layout m_pipeline_layout;
  pipe m_pipeline;

  struct consts {
    float w, h;
  } m_consts {};

public:
  explicit pipeline(const m4c0::fuji::device_context * ld, unsigned w, unsigned h);

  void build_secondary_command_buffer(VkCommandBuffer cb) {
    m4c0::vulkan::cmd::bind_pipeline(cb).with_pipeline(&m_pipeline).now();
    m4c0::vulkan::cmd::push_constants(cb)
        .with_data_from(&m_consts)
        .with_pipeline_layout(&m_pipeline_layout)
        .to_vertex_stage();
  }
};

class pixel_guard {
  decltype(m4c0::vulkan::device_memory().map_all()) m_guard;
  instance * m_ptr;
  int m_pos = 0;

  static constexpr const auto conv = [](auto i) {
    return static_cast<std::int16_t>(i);
  };

  void draw(std::int16_t sx, std::int16_t sy, std::int16_t w, std::int16_t h, std::uint32_t color) {
    m_ptr[m_pos++] = instance { sx, sy, w, h, color };
  }

public:
  explicit pixel_guard(unsigned w, m4c0::vulkan::device_memory * mem)
    : m_guard(mem->map_all())
    , m_ptr(m_guard.pointer<instance>()) {
    draw_bar = [this](int sx, int sy, int w, int h, std::uint32_t color) {
      draw(conv(sx), conv(sy), conv(w), conv(h), color);
    };
  }
};

class dbl_buf {
  std::array<color_mem, 2> m_clr_mem;
  std::unique_ptr<pixel_guard> m_guard;
  color_mem * m_front = &m_clr_mem.at(0);
  color_mem * m_back = &m_clr_mem.at(1);
  std::mutex m_mutex {};

public:
  dbl_buf(const m4c0::fuji::device_context * ld, std::uint16_t w, std::uint16_t h)
    : m_clr_mem({ color_mem { ld, w, h }, color_mem { ld, w, h } }) {
    lock = [this, w, h]() {
      auto guard = std::lock_guard { m_mutex };
      m_guard = std::make_unique<pixel_guard>(w, m_back->device_memory());
    };
    unlock = [this]() {
      auto guard = std::lock_guard { m_mutex };
      m_guard.reset();
      std::swap(m_front, m_back);
    };
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) {
    ddkCalcFrame();
    auto guard = std::lock_guard { m_mutex };
    m_front->build_secondary_command_buffer(cb);
  }

  [[nodiscard]] auto count() {
    auto guard = std::lock_guard { m_mutex };
    return m_front->count();
  }
};

class objects : public m4c0::fuji::main_loop_listener {
  m4c0::vulkan::tools::full_extent_viewport m_viewport;
  pipeline m_pipeline;
  vtx_mem m_vtx_mem;
  dbl_buf m_dbl_buf;

public:
  objects(const m4c0::fuji::device_context * ld, std::uint16_t w, std::uint16_t h)
    : m_viewport()
    , m_pipeline(ld, w, h)
    , m_vtx_mem(ld)
    , m_dbl_buf(ld, w, h) {
  }
  void build_primary_command_buffer(VkCommandBuffer cb) override {
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) override {
    m_viewport.build_command_buffer(cb);
    m_pipeline.build_secondary_command_buffer(cb);
    m_vtx_mem.build_secondary_command_buffer(cb);
    m_dbl_buf.build_secondary_command_buffer(cb);
    m4c0::vulkan::cmd::draw(cb).with_vertex_count(vtx_mem::count()).with_instance_count(m_dbl_buf.count()).now();
  }
  void on_render_extent_change(m4c0::vulkan::extent_2d e) override {
    m_viewport.extent() = e;
  }
};

class native_stuff {
  std::atomic<float> m_mouse_x {};
  std::atomic<float> m_mouse_y {};
  std::atomic_bool m_click {};

protected:
  void update_mouse_down(bool down) {
    m_click = down;
  }
  void update_mouse_pos(float x, float y) {
    m_mouse_x = x;
    m_mouse_y = y;
  }

public:
  void update_mouse(m4c0::vulkan::extent_2d ddk_extent) const {
    mouse_px = mouse_x.load();
    mouse_py = mouse_y.load();

    mouse_x = static_cast<int>(m_mouse_x * static_cast<float>(ddk_extent.width()));
    mouse_y = static_cast<int>(m_mouse_y * static_cast<float>(ddk_extent.height()));

    mouse_leftclick = m_click && !mouse_left;
    mouse_left = m_click.load();
  }
};

class stuff : public m4c0::fuji::main_loop_listener {
  const native_stuff * m_ns;
  std::unique_ptr<objects> m_obj {};
  std::mutex m_obj_mutex {};
  m4c0::vulkan::extent_2d m_render_extent;
  m4c0::vulkan::extent_2d m_ddk_extent;

public:
  explicit stuff(const native_stuff * ns) : m_ns(ns) {
  }
  void build_primary_command_buffer(VkCommandBuffer cb) override {
    auto guard = std::lock_guard { m_obj_mutex };
    if (m_obj) m_obj->build_primary_command_buffer(cb);
    update_mouse();
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) override {
    auto guard = std::lock_guard { m_obj_mutex };
    if (m_obj) m_obj->build_secondary_command_buffer(cb);
  }
  void on_render_extent_change(m4c0::vulkan::extent_2d e) override {
    auto guard = std::lock_guard { m_obj_mutex };
    if (m_obj) m_obj->on_render_extent_change(e);
    m_render_extent = e;
  }

  void reset(const m4c0::fuji::device_context * ld, unsigned w, unsigned h) {
    auto guard = std::lock_guard { m_obj_mutex };
    m_obj = std::make_unique<objects>(ld, w, h);
    m_ddk_extent = m4c0::vulkan::extent_2d { w, h };
  }
  void update_mouse() const {
    m_ns->update_mouse(m_ddk_extent);
  }
};

class ddk_guard {
public:
  explicit ddk_guard(const m4c0::native_handles * np) {
    ddkInit(np);
  }
  ~ddk_guard() {
    ddkFree();
  }

  ddk_guard(const ddk_guard & o) = delete;
  ddk_guard(ddk_guard && o) = delete;
  ddk_guard & operator=(const ddk_guard & o) = delete;
  ddk_guard & operator=(ddk_guard && o) = delete;
};

class loop : public m4c0::fuji::main_loop {
public:
  void run_global(const m4c0::native_handles * nh, const native_stuff * ns) {
    m4c0::fuji::device_context ld { "SFXR", nh };
    stuff s { ns };
    listener() = &s;

    set_screen_size = [this, s = &s, ld = &ld](int w, int h) {
      s->reset(ld, w, h);
      ddkpitch = w;
    };

    ddk_guard ddk { nh };
    run_device(&ld);
  }
};

extern const native_stuff * const natives_ptr;
