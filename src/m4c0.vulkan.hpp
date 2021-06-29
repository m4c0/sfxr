#pragma once

#include "draw_context.hpp"
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
#include "mouse.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <thread>

struct scr_context {
  const m4c0::native_handles * nh;
  const m4c0::fuji::device_context * ld;
  unsigned w, h;
};

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
  explicit color_mem(const scr_context * ctx)
    : m_count(ctx->w * ctx->h)
    , m_buffer(buffer::create_vertex_buffer_with_size(m_count * sizeof(instance)))
    , m_memory(ctx->ld->create_host_memory(&m_buffer))
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
  explicit pipeline(const scr_context * ctx);

  void build_secondary_command_buffer(VkCommandBuffer cb) {
    m4c0::vulkan::cmd::bind_pipeline(cb).with_pipeline(&m_pipeline).now();
    m4c0::vulkan::cmd::push_constants(cb)
        .with_data_from(&m_consts)
        .with_pipeline_layout(&m_pipeline_layout)
        .to_vertex_stage();
  }
};

class pixel_guard : public gui::draw_context {
  decltype(m4c0::vulkan::device_memory().map_all()) m_guard;
  sprite_set * m_font;
  instance * m_ptr;
  gui::mouse * m_mouse;
  int m_pos = 0;

  static constexpr const auto conv = [](auto i) {
    return static_cast<std::int16_t>(i);
  };

  void draw(std::int16_t sx, std::int16_t sy, std::int16_t w, std::int16_t h, std::uint32_t color) {
    m_ptr[m_pos++] = instance { sx, sy, w, h, color };
  }

protected:
  [[nodiscard]] sprite_set * font() override {
    return m_font;
  }

public:
  pixel_guard(m4c0::vulkan::device_memory * mem, sprite_set * font, gui::mouse * mouse)
    : m_guard(mem->map_all())
    , m_ptr(m_guard.pointer<instance>())
    , m_font(font)
    , m_mouse(mouse) {
    draw_bar = [this](int sx, int sy, int w, int h, std::uint32_t color) {
      panel({ { sx, sy }, { w, h } }, color);
    };
  }

  void panel(const gui::rect & r, std::uint32_t color) override {
    draw(conv(r.origin.x), conv(r.origin.y), conv(r.size.w), conv(r.size.h), color);
  }

  gui::mouse * mouse() override {
    return m_mouse;
  }
};

class my_mouse : public gui::mouse {
public:
  bool is_left_down() override {
    return mouse_left;
  }
  bool is_left_down_edge() override {
    return mouse_leftclick;
  }
  gui::point delta() override {
    return { mouse_x - mouse_px, mouse_y - mouse_py };
  }
  gui::point position() override {
    return { mouse_x, mouse_y };
  }
};
class dbl_buf {
  std::array<color_mem, 2> m_clr_mem;
  color_mem * m_front = &m_clr_mem.at(0);
  color_mem * m_back = &m_clr_mem.at(1);
  sprite_set m_font;
  my_mouse m_mouse;

  void calc_frame() {
    pixel_guard pg { m_back->device_memory(), &m_font, &m_mouse };
    if (ddkCalcFrame(&pg)) std::swap(m_front, m_back);
  }

public:
  explicit dbl_buf(const scr_context * ctx)
    : m_clr_mem({ color_mem { ctx }, color_mem { ctx } })
    , m_font(sprite_set::load(ctx->nh, "font", true)) {
  }

  void build_secondary_command_buffer(VkCommandBuffer cb) {
    calc_frame();
    m_front->build_secondary_command_buffer(cb);
  }

  [[nodiscard]] auto count() {
    return m_front->count();
  }
};

class objects : public m4c0::fuji::main_loop_listener {
  m4c0::vulkan::tools::full_extent_viewport m_viewport;
  pipeline m_pipeline;
  vtx_mem m_vtx_mem;
  dbl_buf m_dbl_buf;

public:
  explicit objects(const scr_context * ctx) : m_viewport(), m_pipeline(ctx), m_vtx_mem(ctx->ld), m_dbl_buf(ctx) {
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

  void reset(const scr_context * ctx) {
    auto guard = std::lock_guard { m_obj_mutex };
    m_obj = std::make_unique<objects>(ctx);
    m_ddk_extent = m4c0::vulkan::extent_2d { ctx->w, ctx->h };
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

    set_screen_size = [nh, s = &s, ld = &ld](int w, int h) {
      scr_context ctx { nh, ld, static_cast<unsigned int>(w), static_cast<unsigned int>(h) };
      s->reset(&ctx);
      ddkpitch = w;
    };

    ddk_guard ddk { nh };
    run_device(&ld);
  }
};

extern const native_stuff * const natives_ptr;
