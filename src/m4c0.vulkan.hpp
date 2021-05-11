#pragma once

#include "m4c0/fuji/device_context.hpp"
#include "m4c0/fuji/main_loop.hpp"
#include "m4c0/vulkan/bind_pipeline.hpp"
#include "m4c0/vulkan/bind_vertex_buffer.hpp"
#include "m4c0/vulkan/buffer.hpp"
#include "m4c0/vulkan/buffer_memory_bind.hpp"
#include "m4c0/vulkan/device_memory.hpp"
#include "m4c0/vulkan/draw.hpp"
#include "m4c0/vulkan/full_extent_viewport.hpp"
#include "m4c0/vulkan/pipeline.hpp"
#include "m4c0/vulkan/pipeline_layout.hpp"
#include "m4c0/vulkan/shader_module.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

extern "C" {
#include "main.frag.h"
#include "main.vert.h"
}

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
      std::array { pos { -1, -1 }, pos { -1, 1 }, pos { 1, -1 }, pos { 1, -1 }, pos { -1, 1 }, pos { 1, 1 } };

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

class color_mem {
  using buffer = m4c0::vulkan::buffer;
  using memory = m4c0::vulkan::device_memory;
  using bind = m4c0::vulkan::tools::buffer_memory_bind;

  buffer m_buffer;
  memory m_memory;
  bind m_bind;

public:
  color_mem(const m4c0::fuji::device_context * ld, std::uint16_t w, std::uint16_t h)
    : m_buffer(buffer::create_vertex_buffer_with_size(w * h * 4))
    , m_memory(ld->create_host_memory(&m_buffer))
    , m_bind(&m_buffer, &m_memory) {
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) {
    m4c0::vulkan::cmd::bind_vertex_buffer(cb).with_buffer(&m_buffer).with_first_index(1).now();
  }
};

class pipeline {
  using pipe = m4c0::vulkan::pipeline;
  using pipeline_layout = m4c0::vulkan::pipeline_layout;

  pipeline_layout m_pipeline_layout;
  pipe m_pipeline;

public:
  explicit pipeline(const m4c0::fuji::device_context * ld)
    : m_pipeline_layout(pipeline_layout::builder().build())
    , m_pipeline(pipe::builder()
                     .with_pipeline_layout(&m_pipeline_layout)
                     .with_render_pass(ld->render_pass())
                     .add_vertex_binding_with_stride(vtx_mem::stride())
                     .add_vec2_attribute_with_bind_and_offset(0, 0)
                     .add_vertex_stage(shader(main_vert_spv, main_vert_spv_len), "main")
                     .add_fragment_stage(shader(main_frag_spv, main_frag_spv_len), "main")
                     .build()) {
  }

  void build_secondary_command_buffer(VkCommandBuffer cb) {
    m4c0::vulkan::cmd::bind_pipeline(cb).with_pipeline(&m_pipeline).now();
  }
};

class objects : public m4c0::fuji::main_loop_listener {
  m4c0::vulkan::tools::full_extent_viewport m_viewport;
  pipeline m_pipeline;
  vtx_mem m_vtx_mem;
  color_mem m_clr_mem;

public:
  objects(const m4c0::fuji::device_context * ld, unsigned w, unsigned h)
    : m_viewport()
    , m_pipeline(ld)
    , m_vtx_mem(ld)
    , m_clr_mem(ld, w, h) {
  }
  void build_primary_command_buffer(VkCommandBuffer cb) override {
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) override {
    m_viewport.build_command_buffer(cb);
    m_pipeline.build_secondary_command_buffer(cb);
    m_vtx_mem.build_secondary_command_buffer(cb);
    // m_clr_mem.build_secondary_command_buffer(cb);
    m4c0::vulkan::cmd::draw(cb).with_vertex_count(vtx_mem::count()).now();
  }
  void on_render_extent_change(m4c0::vulkan::extent_2d e) override {
    m_viewport.extent() = e;
  }
};

class stuff : public m4c0::fuji::main_loop_listener {
  const m4c0::fuji::device_context * m_ctx;
  std::unique_ptr<objects> m_obj {};

public:
  explicit stuff(const m4c0::fuji::device_context * ld) : m_ctx(ld) {
    create_objects(640, 480);
  }
  void build_primary_command_buffer(VkCommandBuffer cb) override {
    if (m_obj) m_obj->build_primary_command_buffer(cb);
  }
  void build_secondary_command_buffer(VkCommandBuffer cb) override {
    if (m_obj) m_obj->build_secondary_command_buffer(cb);
  }
  void on_render_extent_change(m4c0::vulkan::extent_2d e) override {
    if (m_obj) m_obj->on_render_extent_change(e);
  }

  void create_objects(unsigned w, unsigned h) {
    m_obj = std::make_unique<objects>(m_ctx, w, h);
  }
};
